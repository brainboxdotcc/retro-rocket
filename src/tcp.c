#include <kernel.h>

static struct hashmap* tcb = NULL;
static tcp_conn_t* fd[FD_MAX] = { 0 };

/* Must match up with order and amount of error messages in tcp_error_code_t */
static const char* error_messages[] = {
	"Socket is already closing",
	"Port in use",
	"Network is down",
	"Invalid connection",
	"Write too large",
	"Socket not connected",
	"Out of socket descriptors",
	"Out of memory",
	"Invalid socket descriptor",
	"Connection failed",
};

/* Set this to output or record a trace of the TCP I/O. This is very noisy! */
#define TCP_TRACE 1

bool tcp_state_receive_fin(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len);

const char* socket_error(int error_code)
{
	if (error_code <= TCP_LAST_ERROR || error_code >= 0) {
		return "No error";
	}
	error_code = abs(error_code) - 1;
	return error_messages[error_code];
}

uint32_t get_isn()
{
	return (time(NULL) * 1000) - (get_ticks() % 1000);
}

int tcp_allocate_fd(tcp_conn_t* conn)
{
	for (size_t x = 0; x < FD_MAX; ++x) {
		if (fd[x] == NULL) {
			fd[x] = conn;
			return x;
		}
	}
	return -1;
}

void tcp_free_fd(int x)
{
	if (x >= 0 && x < FD_MAX) {
		fd[x] = NULL;
	}
}

tcp_conn_t* tcp_find_by_fd(int x)
{
	if (x >= 0 && x < FD_MAX) {
		return fd[x];
	}
	return NULL;
}

bool seq_lt(uint32_t x, uint32_t y) 
{
	return ((int)(x - y) < 0);
}

bool seq_lte(uint32_t x, uint32_t y)
{
	return ((int)(x - y) <= 0);
}

bool seq_gt(uint32_t x, uint32_t y)
{
	return ((int)(x - y) > 0);
}

bool seq_gte(uint32_t x, uint32_t y)
{
	return ((int)(x - y) >= 0);
}

/**
 * @brief Comparison function for hash table of tcp connections
 * 
 * @param a first object to compare
 * @param b second object to compare
 * @param udata user data
 * @return int 0 for equal, 1 for not equal
 */
int tcp_conn_compare(const void *a, const void *b, void *udata) {
	const tcp_conn_t* fa = a;
	const tcp_conn_t* fb = b;
	return fa->local_addr == fb->local_addr && fa->remote_addr && fb->remote_addr
		&& fa->local_port == fb->local_port && fa->remote_port == fb->remote_port ? 0 : 1;
}

/**
 * @brief Hash an ip connection
 * 
 * @param item item to hash
 * @param seed0 first seed from hashmap
 * @param seed1 second seed from hashmap
 * @return uint64_t hash bucket value
 */
uint64_t tcp_conn_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const tcp_conn_t* fa = item;
	return ((uint64_t)fa->local_addr << 32) + ((uint64_t)fa->remote_addr);
}

void tcp_free(tcp_conn_t* conn)
{
	// Delete any outstanding segments
	for (tcp_ordered_list_t* t = conn->segment_list; t; t = t->next) {
		kfree(t->segment);
		kfree(t);
	}
	conn->segment_list = NULL;
	// Remove the TCB
	hashmap_delete(tcb, conn);
}

uint16_t tcp_calculate_checksum(ip_packet_t* packet, tcp_segment_t* segment, size_t len)
{
	int array_size = len + sizeof(tcp_ip_pseudo_header_t);
	tcp_ip_pseudo_header_t* pseudo = kmalloc(array_size * 2);
	array_size += (array_size % 2); // Ensure buffer is even-aligned
	memset(pseudo, 0, array_size * 2);
	pseudo->dst = *((uint32_t*)&packet->dst_ip);
	pseudo->src = *((uint32_t*)&packet->src_ip);
	pseudo->protocol = PROTOCOL_TCP;
	pseudo->reserved = 0;
	pseudo->len = htons(len);
	uint32_t checksum = segment->checksum;
	segment->checksum = 0;
	memcpy(pseudo->body, segment, len);
	segment->checksum = checksum;

	// Treat the packet header as a 2-byte-integer array
	// Sum all integers switch to network byte order
	uint16_t * array = (uint16_t*)pseudo;
	uint32_t sum = 0;
	for(int i = 0; i < array_size / 2; i++) {
		sum += htons(array[i]);
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);

	kfree(pseudo);
	return ~sum;
}

/**
 * @brief Dump debug info for a TCP segment
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param options TCP options
 * @param len TCP length
 * @param our_checksum calculated checksum
 */
void tcp_dump_segment(bool in, tcp_conn_t* conn, const ip_packet_t* encap_packet, const tcp_segment_t* segment, const tcp_options_t* options, size_t len, uint16_t our_checksum)
{
#if (TCP_TRACE != 1)
	return;
#endif
	const char *states[] = { "LISTEN","SYN-SENT","SYN-RECEIVED","ESTABLISHED","FIN-WAIT-1","FIN-WAIT-2","CLOSE-WAIT","CLOSING","LAST-ACK","TIME-WAIT" };
	char source_ip[15] = { 0 }, dest_ip[15] = { 0 };
	setforeground(current_console, our_checksum == segment->checksum ? COLOUR_LIGHTGREEN : COLOUR_LIGHTRED);
	get_ip_str(source_ip, encap_packet->src_ip);
	get_ip_str(dest_ip, encap_packet->dst_ip);
	dprintf(
		"TCP %s: %s %s:%d->%s:%d len=%ld seq=%d ack=%d off=%d flags[%c%c%c%c%c%c%c%c] win=%d, sum=%04x/%04x, urg=%d",
		in ? "IN" : "OUT",
		conn ? states[conn->state] : "CLOSED",
		source_ip,
		segment->src_port,
		dest_ip,
		segment->dst_port,
		len,
		segment->seq,
		segment->ack,
		segment->flags.off,
		segment->flags.fin ? 'F' : '-',
		segment->flags.syn ? 'S' : '-',
		segment->flags.rst ? 'R' : '-',
		segment->flags.psh ? 'P' : '-',
		segment->flags.ack ? 'A' : '-',
		segment->flags.urg ? 'U' : '-',
		segment->flags.ece ? 'E' : '-',
		segment->flags.cwr ? 'C' : '-',
		segment->window_size,
		segment->checksum,
		our_checksum,
		segment->urgent
	);
	if (options && options->mss) {
		dprintf(" [opt.mss=%d]", options->mss);
	}
	dprintf("\n");
	setforeground(current_console, COLOUR_WHITE);
}

/**
 * @brief Swap byte order of an inbound packet fields to host byte order
 * 
 * @param segment segment to swap byte order for
 */
void tcp_byte_order_in(tcp_segment_t* const segment)
{
	segment->src_port = ntohs(segment->src_port);
	segment->dst_port = ntohs(segment->dst_port);
	segment->seq = ntohl(segment->seq);
	segment->ack = ntohl(segment->ack);
	segment->window_size = ntohs(segment->window_size);
	segment->checksum = ntohs(segment->checksum);
	segment->urgent = ntohs(segment->urgent);
}

void tcp_byte_order_out(tcp_segment_t* const segment)
{
	uint8_t tmp;
	segment->src_port = htons(segment->src_port);
	segment->dst_port = htons(segment->dst_port);
	segment->seq = htonl(segment->seq);
	segment->ack = htonl(segment->ack);
	segment->window_size = htons(segment->window_size);
	segment->checksum = htons(segment->checksum);
	segment->urgent = htons(segment->urgent);
	tmp = segment->flags.bits1;
	segment->flags.bits1 = segment->flags.bits2;
	segment->flags.bits2 = tmp;
}


/**
 * @brief Parse the options fields of a tcp segment into a
 * tcp_options_t struct.
 * 
 * @param segment segment to parse
 * @param options options struct to fill
 * @return number of recognised options found
 */
uint8_t tcp_parse_options(tcp_segment_t* const segment, tcp_options_t* options)
{
	uint8_t* opt_ptr = segment->options;
	uint8_t n_opts = 0;
	options->mss = 0;

	while (opt_ptr < (uint8_t*)segment + (segment->flags.off * 4) && *opt_ptr != TCP_OPT_END) {
		uint8_t opt_size = *(opt_ptr + 1);
		switch (*opt_ptr) {
			case TCP_OPT_MSS:
				n_opts++;
				options->mss = ntohs(*((uint16_t*)(opt_ptr + 2)));
			break;
			default:
			break;
		}
		opt_ptr += (opt_size ? opt_size : 1);
	}
	return n_opts;
}

/**
 * @brief Find a TCP TCB by source and destination address/port pairs
 * 
 * @param source_addr source address
 * @param dest_addr destination addres
 * @param source_port source port
 * @param dest_port destination port
 * @return tcp_conn_t* TCB
 */
tcp_conn_t* tcp_find(uint32_t source_addr, uint32_t dest_addr, uint16_t source_port, uint16_t dest_port)
{
	if (tcb == NULL) {
		return NULL;
	}
	tcp_conn_t find_conn = { .local_addr = source_addr, .remote_addr = dest_addr, .local_port = source_port, .remote_port = dest_port };
	return (tcp_conn_t*)hashmap_get(tcb, &find_conn);
}

/**
 * @brief Set the state for a TCB
 * 
 * @param conn TCB
 * @param new_state new state to set
 * @return tcp_conn_t* modified TCB
 */
tcp_conn_t* tcp_set_state(tcp_conn_t* conn, tcp_state_t new_state)
{
	if (conn == NULL) {
		return NULL;
	}
	conn->state = new_state;
	return conn;
}

/**
 * @brief Build the options for a segment before sending
 * 
 * @param options options pointer to fill
 * @param opt options values to set
 * @return uint8_t number of bytes filled
 */
uint8_t tcp_build_options(uint8_t* options, const tcp_options_t* opt)
{
	uint8_t index = 0;
	if (opt->mss) {
		options[index++] = TCP_OPT_MSS;
		options[index++] = 4;
		options[index++] = opt->mss / 0xFF; // network order, MSB
		options[index++] = opt->mss % 0xFF; // network order, LSB
	}
	return index;
}

/**
 * @brief Send a segment to the peer
 * 
 * @param conn TCB
 * @param seq send sequence number
 * @param flags header flags
 * @param data data to send
 * @param count number of octets of data to send
 * @return tcp_conn_t* modified TCB
 */
tcp_conn_t* tcp_send_segment(tcp_conn_t *conn, uint32_t seq, uint8_t flags, const void *data, size_t count)
{
	if (conn == NULL) {
		dprintf("TCP: Refusing to send segment on null conn\n");
		return NULL;
	}

	dprintf("TCP send segment\n");

	ip_packet_t encap;
	tcp_options_t options = { .mss = flags & TCP_SYN ? 1460 : 0 };
	uint16_t length = sizeof(tcp_segment_t) + count + (flags & TCP_SYN ? 4 : 0);
	tcp_segment_t * packet = kmalloc(length);

	memset(packet, 0, length);
	packet->src_port = conn->local_port;
	packet->dst_port = conn->remote_port;
	packet->seq = seq;
	packet->ack = (flags & TCP_ACK) ? conn->rcv_nxt : 0;
	packet->flags.bits2 = 0;
	packet->flags.bits1 = 0;
	// Account for options sent with SYN
	packet->flags.off = (flags & TCP_SYN) ? TCP_PACKET_SIZE_OFF + 1 : TCP_PACKET_SIZE_OFF;
	// Set flags
	packet->flags.syn = (flags & TCP_SYN) ? 1 : 0;
	packet->flags.fin = (flags & TCP_FIN) ? 1 : 0;
	packet->flags.ack = (flags & TCP_ACK) ? 1 : 0;
	packet->flags.rst = (flags & TCP_RST) ? 1 : 0;
	packet->flags.psh = (flags & TCP_PSH) ? 1 : 0;
	packet->flags.urg = (flags & TCP_URG) ? 1 : 0;
	packet->flags.ece = (flags & TCP_ECE) ? 1 : 0;
	packet->flags.cwr = (flags & TCP_CWR) ? 1 : 0;

	packet->window_size = TCP_WINDOW_SIZE;
	packet->checksum = 0;
	packet->urgent = 0;
	memcpy(&encap.src_ip, &conn->local_addr, 4);
	memcpy(&encap.dst_ip, &conn->remote_addr, 4);
	tcp_byte_order_out(packet);
	tcp_build_options(packet->options, &options);

	// Copy data over
	memcpy((void*)packet->payload + (flags & TCP_SYN ? 4 : 0), data, count);

	packet->checksum = htons(tcp_calculate_checksum(&encap, packet, length));
	dprintf("TCP -> IP send packet\n");
	ip_send_packet(encap.dst_ip, packet, length, PROTOCOL_TCP);
	dprintf("TCP -> IP send packet DONE\n");

	conn->snd_nxt += count;
	if (flags & (TCP_SYN | TCP_FIN)) {
		++conn->snd_nxt;
	}

	tcp_byte_order_in(packet);
	tcp_dump_segment(false, conn, &encap, packet, &options, length, packet->checksum);

	kfree(packet);

	return conn;
}

/**
 * @brief Calculate size of header for a segment using the payload offset
 * 
 * @param s TCP segment
 * @return size_t size of header
 */
size_t tcp_header_size(tcp_segment_t* s)
{
	return (s->flags.off * 4);
}

/**
 * @brief Write data to an open connection
 * 
 * @param conn TCB
 * @param data data to send
 * @param count number of octets to send
 * @return int zero on success, negative value on failure
 */
int tcp_write(tcp_conn_t* conn, const void* data, size_t count)
{
	if (conn == NULL) {
		return TCP_ERROR_INVALID_CONNECTION;
	} else if (count > TCP_WINDOW_SIZE) {
		return TCP_ERROR_WRITE_TOO_LARGE;
	} else if (conn->state != TCP_ESTABLISHED) {
		return TCP_ERROR_NOT_CONNECTED;
	}
	tcp_send_segment(conn, conn->snd_nxt, TCP_ACK | TCP_PSH, data, count);
	return 0;
}

/**
 * @brief Insert segment into list of segments ordered by sequence number.
 * If this segment partially overlaps a segment in the list, that segment will be
 * adjusted in size and this segment will overwrite it. If however the current
 * segment's sequence number exists exactly in the list this is a duplicate
 * segment and will be dropped.
 * 
 * @param conn tcp connection
 * @param segment tcp segment
 * @param len length of tcp segment payload
 * @return tcp_segment_t* newly inserted copy of segment on success, or NULL
 */
tcp_segment_t* tcp_ord_list_insert(tcp_conn_t* conn, tcp_segment_t* segment, size_t len)
{
	tcp_ordered_list_t* cur = NULL, *next_ord = NULL;
	tcp_segment_t *prev = NULL;

	for (cur = conn->segment_list; cur; cur = cur->next) {
		if (seq_lte(segment->seq, cur->segment->seq)) {
			break;
		}
	}

	if (cur != NULL && cur->prev != NULL) {
		prev = cur->prev->segment;
		size_t prev_end = prev->seq + cur->prev->len;
		if (seq_gte(prev_end, segment->seq + len)) {
			// Packet overlaps this packet, drop this packet
			return NULL;
		} else if (seq_gt(prev_end, segment->seq)) {
			// cut off end to fit segments together, this partially overwrites the other
			cur->prev->len -= prev_end + segment->seq;
		}
	}

	// If we receive a FIN, clear the list of any packets after this one and free memory
	if (segment->flags.fin && cur != NULL) {
		while (cur) {
			next_ord = cur->next;
			kfree(cur->segment);
			kfree(cur);
			cur = next_ord;
		}
	}

	while (cur) {
		size_t seg_end = segment->seq + len;
		size_t cur_end = cur->segment->seq + cur->len;

		if (seq_lt(seg_end, cur->segment->seq)) {
			// No overlap
			break;
		}

		if (seq_lt(seg_end, cur_end)) {
			len -= seg_end - cur->segment->seq;
		}

		/* Remove element */
		next_ord = cur->next;
		if (cur->prev != NULL) {
			cur->prev->next = cur->next;
		}
		if (cur->next != NULL) {
			cur->next->prev = cur->prev;
		}
		if (cur->next == NULL && cur->prev == NULL) {
			conn->segment_list = NULL;
		} else if (cur != NULL && cur == conn->segment_list) {
			conn->segment_list = cur->next;
		}
		kfree(cur->segment);
		kfree(cur);

		cur = next_ord;
	}

	tcp_ordered_list_t* new = kmalloc(sizeof(tcp_ordered_list_t));
	new->segment = kmalloc(len + tcp_header_size(segment));
	memcpy(new->segment, segment, len + tcp_header_size(segment));
	new->len = len;
	new->next = NULL;
	new->prev = NULL;

	if (cur) {
		if (cur->prev != NULL) {
			// Not first item
			cur->prev->next = new;
			new->prev = cur->prev;
		}
		if (cur->next != NULL) {
			// Not last item
			cur->next->prev = new;
			new->next = cur->next;
		}
		if (new->prev == NULL) {
			// First item, replace list ptr
			conn->segment_list = new;
		}
	} else {
		// Only item
		conn->segment_list = new;
	}

	return new->segment;
}

/**
 * @brief Send completed sequences of segments to the recv buffer
 * 
 * @param conn TCB
 * @param segment TCP segment
 * @param len length of segment
 */
void tcp_process_queue(tcp_conn_t* conn, tcp_segment_t* segment, size_t len)
{
	tcp_ordered_list_t* cur;
	for (cur = conn->segment_list; cur; cur = cur->next) {
		if (conn->rcv_nxt != cur->segment->seq) {
			break;
		}
		// This is now received data, accounting for options
		uint8_t* payload = cur->segment->payload;
		size_t payload_len = cur->len;
		if (cur->segment->flags.off > TCP_PACKET_SIZE_OFF) {
			size_t options_len = ((cur->segment->flags.off - TCP_PACKET_SIZE_OFF) * 4);
			payload += options_len;
			payload_len -= options_len;
		}

		// increment what we have received in our connection state
		conn->rcv_nxt += payload_len;

		if (payload_len > 0) {
			// Add received data to the high level receive buffer
			while (conn->recv_buffer_spinlock);
			conn->recv_buffer_spinlock++;
			conn->recv_buffer = krealloc(conn->recv_buffer, payload_len + conn->recv_buffer_len);
			memcpy(conn->recv_buffer + conn->recv_buffer_len, payload, payload_len);
			conn->recv_buffer_len += payload_len;
			conn->recv_buffer_spinlock--;
		}

		// Remove from doubly linked list
		if (cur->prev != NULL) {
			cur->prev->next = cur->next;
		}
		if (cur->next != NULL) {
			cur->next->prev = cur->prev;
		}
		if (cur->next == NULL && cur->prev == NULL) {
			conn->segment_list = NULL;
		} else if (cur != NULL && cur == conn->segment_list) {
			conn->segment_list = cur->next;
		}
		kfree(cur->segment);
		kfree(cur);
	}
}

bool tcp_state_listen(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	// TODO - implement this
	return true;
}

/**
 * @brief Send an ACK for the segments we have received so far
 * 
 * @param conn 
 * @return true on successfully queueing the segment
 */
bool tcp_send_ack(tcp_conn_t* conn)
{
	/* Check for duplicate ACKs (same rcv_nxt as the previous) and drop them */
	if (conn->rcv_nxt == conn->rcv_lst) {
		return true;
	}
	conn->snd_lst = conn->snd_nxt;
	conn->rcv_lst = conn->rcv_nxt;
	return tcp_send_segment(conn, conn->snd_nxt, TCP_ACK, NULL, 0);
}

/**
 * @brief Called when state is SYN-SENT
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_syn_sent(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	if (segment->flags.ack) {
		if (seq_lte(segment->ack, conn->iss) || seq_gt(segment->ack, conn->snd_nxt)) {
			if (!segment->flags.rst) {
				tcp_send_segment(conn, segment->ack, TCP_RST, NULL, 0);
			}
		}
	}

	if (segment->flags.rst) {
		if (segment->flags.ack) {
			// TOD: Connection reset
		}

		return true;
	}

	if (segment->flags.syn) {
		conn->irs = segment->seq;
		conn->rcv_nxt = segment->seq + 1;

		if (segment->flags.ack) {
			conn->snd_una = segment->ack;
			conn->snd_wnd = segment->window_size;
			conn->snd_wl1 = segment->seq;
			conn->snd_wl2 = segment->ack;

			// TODO - Segments on the retransmission queue which are ack'd should be removed

			tcp_set_state(conn, TCP_ESTABLISHED);
			tcp_send_ack(conn);

			// TODO - Data queued for transmission may be included with the ACK.
			// TODO - If there is data in the segment, continue processing at the URG phase.
		} else {
			tcp_set_state(conn, TCP_SYN_RECEIVED);

			--conn->snd_nxt;
			tcp_send_segment(conn, conn->snd_nxt, TCP_SYN | TCP_ACK, NULL, 0);
		}
	}
	return true;
}

/**
 * @brief Called when state is SYN-RECEIVED
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_syn_received(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
 	if (segment->flags.fin) {
		tcp_state_receive_fin(encap_packet, segment, conn, options, len);
	}
	return true;
}

/**
 * @brief Called when we receive RST
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_receive_rst(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	switch (conn->state) {
		case TCP_SYN_RECEIVED:
			// Connection refused
			dprintf("*** TCP *** Connection refused\n");
			tcp_free(conn);
			break;
		case TCP_ESTABLISHED:
		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
		case TCP_CLOSE_WAIT:
			// Connection reset by peer
			dprintf("*** TCP *** Connection reset by peer\n");
			tcp_free(conn);
			break;
		case TCP_CLOSING:
		case TCP_LAST_ACK:
		case TCP_TIME_WAIT:
			// Connection closed
			dprintf("*** TCP *** Connection gracefully closed\n");
			tcp_free(conn);
			break;
		default:
			break;
	}
	return false;
}

/**
 * @brief Called for general data IN that we need to ACK
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_handle_data_in(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	size_t payload_len = len - tcp_header_size(segment);

	if (!(seq_lte(conn->rcv_nxt, segment->seq) && seq_lte(segment->seq + payload_len, conn->rcv_nxt + conn->rcv_wnd))) {
		// Unacceptable segment
		if (segment->flags.rst == 0) {
			return tcp_send_ack(conn);
		}
		return false;
	}

	// Check RST bit
	if (segment->flags.rst) {
		return tcp_state_receive_rst(encap_packet, segment, conn, options, len);
	}

	// insert packet into ordered list
	tcp_ord_list_insert(conn, segment, payload_len);
	// check ordered list is complete (no seq gaps), if it is deliver queued data to client
	tcp_process_queue(conn, segment, payload_len);
	// acknowlege receipt
	tcp_send_ack(conn);
	return true;
}

/**
 * @brief Set connection timeout (12 seconds)
 * 
 * @param conn TCB
 */
void tcp_set_conn_msl_time(tcp_conn_t* conn)
{
	conn->msl_time = time(NULL) + 12000;
}

/**
 * @brief Called when we receive FIN
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_receive_fin(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	conn->rcv_nxt = segment->seq + 1;
	tcp_send_ack(conn);

	switch (conn->state) {
		case TCP_SYN_RECEIVED:
		case TCP_ESTABLISHED:
			conn->recv_eof_pos = conn->recv_buffer_len;
			//dump_hex(conn->recv_buffer, conn->recv_buffer_len);
			tcp_set_state(conn, TCP_CLOSE_WAIT);
			break;
		case TCP_FIN_WAIT_1:
			if (seq_gte(segment->ack, conn->snd_nxt)) {
				tcp_set_state(conn, TCP_TIME_WAIT);
				tcp_set_conn_msl_time(conn);
			} else {
				tcp_set_state(conn, TCP_CLOSING);
			}
			break;
		case TCP_FIN_WAIT_2:
			tcp_set_state(conn, TCP_TIME_WAIT);
			tcp_set_conn_msl_time(conn);
			break;
		case TCP_CLOSE_WAIT:
		case TCP_CLOSING:
		case TCP_LAST_ACK:
			break;
		case TCP_TIME_WAIT:
			tcp_set_conn_msl_time(conn);
			break;
		default:
			break;
	}
	return true;
}

/**
 * @brief Called when state is ESTABLISHED
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_established(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	if (len - tcp_header_size(segment) > 0) {
		tcp_handle_data_in(encap_packet, segment, conn, options, len);
	}
 	if (segment->flags.fin) {
		tcp_state_receive_fin(encap_packet, segment, conn, options, len);
	}
	return true;
}

/**
 * @brief Called when state is FIN-WAIT-1
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_fin_wait_1(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_handle_data_in(encap_packet, segment, conn, options, len);
	return true;
}

/**
 * @brief Called when state is FIN-WAIT-2
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_fin_wait_2(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_handle_data_in(encap_packet, segment, conn, options, len);
	return true;
}

/**
 * @brief Called when state is CLOSE-WAIT
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_close_wait(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
        tcp_set_state(conn, TCP_LAST_ACK);
	return true;
}

/**
 * @brief Called when state is CLOSING
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_closing(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

/**
 * @brief Called when state is LAST-ACK
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_last_ack(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

/**
 * @brief Called when state is TIME-WAIT
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_time_wait(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
 	if (segment->flags.fin) {
		tcp_state_receive_fin(encap_packet, segment, conn, options, len);
	}
	return true;
}

/**
 * @brief Finite state machine to route segments based on current state
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param conn TCB
 * @param options options, e.g. MSS
 * @param len length of segment
 * @return true if we are to continue processing
 */
bool tcp_state_machine(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	switch (conn->state) {
		case TCP_LISTEN:
			return tcp_state_listen(encap_packet, segment, conn, options, len);
		case TCP_SYN_SENT:
			return tcp_state_syn_sent(encap_packet, segment, conn, options, len);
		case TCP_SYN_RECEIVED:
			return tcp_state_syn_received(encap_packet, segment, conn, options, len);
		case TCP_ESTABLISHED:
			return tcp_state_established(encap_packet, segment, conn, options, len);
		case TCP_FIN_WAIT_1:
			return tcp_state_fin_wait_1(encap_packet, segment, conn, options, len);
		case TCP_FIN_WAIT_2:
			return tcp_state_fin_wait_2(encap_packet, segment, conn, options, len);
		case TCP_CLOSE_WAIT:
			return tcp_state_close_wait(encap_packet, segment, conn, options, len);
		case TCP_CLOSING:
			return tcp_state_closing(encap_packet, segment, conn, options, len);
		case TCP_LAST_ACK:
			return tcp_state_last_ack(encap_packet, segment, conn, options, len);
		case TCP_TIME_WAIT:
			return tcp_state_time_wait(encap_packet, segment, conn, options, len);
		default:
			return false;
	}
}

/**
 * @brief Handle inbound packet from IP layer
 * 
 * @param encap_packet encapsulating IP packet
 * @param segment TCP segment
 * @param len length of segment
 */
void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len)
{
	tcp_options_t options;
	uint16_t our_checksum = tcp_calculate_checksum(encap_packet, segment, len);
	tcp_byte_order_in(segment);
	tcp_parse_options(segment, &options);
	if (our_checksum == segment->checksum) {
		tcp_conn_t* conn = tcp_find(*((uint32_t*)(&encap_packet->dst_ip)), *((uint32_t*)(&encap_packet->src_ip)), segment->dst_port, segment->src_port);
		if (conn) {
			tcp_dump_segment(true, conn, encap_packet, segment, &options, len, our_checksum);
			tcp_state_machine(encap_packet, segment, conn, &options, len);
		}
	} else {
		tcp_dump_segment(true, NULL, encap_packet, segment, &options, len, our_checksum);
	}
}

/**
 * @brief ISR idle task
 */
void tcp_idle()
{
	if (tcb == NULL) {
		return;
	}

	void *item;
	size_t iter = 0;
	while (hashmap_iter(tcb, &iter, &item)) {
		tcp_conn_t *conn = item;
		if (conn->state == TCP_ESTABLISHED) {
			if (conn->send_buffer_len > 0 && conn->send_buffer != NULL) {
				dprintf("send buffer process wait spin\n");
				while (conn->send_buffer_spinlock);
				conn->send_buffer_spinlock++;
				dprintf("send buffer process done spin\n");
				/* There is buffered data to send from high level functions */
				size_t amount_to_send = conn->send_buffer_len > 1460 ? 1460 : conn->send_buffer_len;
				dprintf("begin tcp write\n");
				tcp_write(conn, conn->send_buffer, amount_to_send);
				dprintf("done tcp write\n");
				/* Resize send buffer down */
				if (conn->send_buffer_len - amount_to_send <= 0) {
					kfree(conn->send_buffer);
					conn->send_buffer = NULL;
				} else {
					conn->send_buffer = krealloc(conn->send_buffer + amount_to_send, conn->send_buffer_len - amount_to_send);
				}
				conn->send_buffer_len -= amount_to_send;
				conn->send_buffer_spinlock--;
				dprintf("done send buffer resize down\n");
			}
		} else if (conn->state == TCP_TIME_WAIT && seq_gte(get_isn(), conn->msl_time)) {
			tcp_free(conn);
			break;
		}
	}
}

/**
 * @brief Initialise TCP
 */
void tcp_init()
{
	tcb = hashmap_new(sizeof(tcp_conn_t), 0, 6, 28, tcp_conn_hash, tcp_conn_compare, NULL, NULL);
	proc_register_idle(tcp_idle, IDLE_FOREGROUND);
}

/**
 * @brief Return true if port is in use on this local address
 * 
 * @param addr local address
 * @param port port number
 * @param type port type, local or remote
 * @return true if port is in use
 */
bool tcp_port_in_use(uint32_t addr, uint16_t port, tcp_port_type_t type)
{
	void *item;
	size_t iter = 0;
	while (hashmap_iter(tcb, &iter, &item)) {
		const tcp_conn_t *conn = item;
		if (conn->local_addr == addr && ((type == TCP_PORT_LOCAL && conn->local_port == port) || (type == TCP_PORT_REMOTE && conn->remote_port == port))) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Allocate a free port >= 1024
 * 
 * @param source_addr source address
 * @param port port, 0 to let the system choose
 * @param type type of port, local or remote
 * @return uint16_t selected port, or 0 if none available
 */
uint16_t tcp_alloc_port(uint32_t source_addr, uint16_t port, tcp_port_type_t type)
{
	if (port == 0) {
		/* Let the OS allocate a port
		 * Walk up the port table, looking for one that doesn't have
		 * any TCB bound to it. If we wrap back around to 0, then
		 * there are no free ports remaining to bind to.
		 */
		for(port = 1024; port != 0; ++port) {
			if (!tcp_port_in_use(source_addr, port, type)) {
				break;
			}

		}
		if (port == 0) {
			return 0;
		}
	}
	return port;
}

int tcp_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port)
{
	tcp_conn_t conn;
	uint32_t isn = get_isn();
	unsigned char ip[4] = { 0 };

	dprintf("tcp_connect() with isn=%d\n", isn);

	gethostaddr(ip);

	conn.remote_addr = target_addr;
	conn.remote_port = target_port;
	conn.local_addr = *((uint32_t*)&ip);

	if (tcp_port_in_use(conn.local_addr, source_port, TCP_PORT_LOCAL)) {
		dprintf("tcp_connect() port in use\n");
		return TCP_ERROR_PORT_IN_USE;
	}

	if (conn.local_addr == 0) {
		dprintf("tcp_connect() net down\n");
		return TCP_ERROR_NETWORK_DOWN;
	}

	conn.local_port = tcp_alloc_port(conn.local_addr, source_port, TCP_PORT_LOCAL);
	conn.snd_una = isn;
	conn.snd_nxt = isn;
	conn.snd_lst = conn.rcv_lst = 0;
	conn.iss = isn;
	conn.snd_wnd = TCP_WINDOW_SIZE;
	conn.snd_up = 0;
	conn.snd_wl1 = 0;
	conn.snd_wl2 = 0;
	conn.rcv_nxt = 0;
	conn.rcv_wnd = TCP_WINDOW_SIZE;
	conn.rcv_up = 0;
	conn.irs = 0;
	conn.fd = -1;
	conn.segment_list = NULL;
	conn.recv_eof_pos = -1;
	conn.send_eof_pos = -1;
	conn.recv_buffer = NULL;
	conn.send_buffer = NULL;
	conn.recv_buffer_len = 0;
	conn.send_buffer_len = 0;
	conn.recv_buffer_spinlock = 0;
	conn.send_buffer_spinlock = 0;

	dprintf("tcp_connect() local port=%d\n", conn.local_port);

	tcp_set_state(&conn, TCP_SYN_SENT);

	dprintf("tcp_connect() sending segment\n");

	tcp_send_segment(&conn, conn.snd_nxt, TCP_SYN, NULL, 0);

	dprintf("tcp_connect() setting hashmap\n");

	hashmap_set(tcb, &conn);

	tcp_conn_t* new_conn = tcp_find(conn.local_addr, conn.remote_addr, conn.local_port, conn.remote_port);
	if (new_conn) {
		dprintf("tcp_connect() setting conn fd\n");
		new_conn->fd = tcp_allocate_fd(new_conn);
		if (new_conn->fd == -1) {
			dprintf("tcp_connect() allocation of fd failed\n");
			tcp_free(new_conn);
			return TCP_ERROR_OUT_OF_DESCRIPTORS;
		}
		return new_conn->fd;
	} else {
		return TCP_ERROR_OUT_OF_MEMORY;
	}
}

int tcp_close(tcp_conn_t* conn)
{
	if (conn == NULL) {
		return TCP_ERROR_INVALID_CONNECTION;
	}
	switch (conn->state) {
		case TCP_LISTEN:
		case TCP_SYN_SENT:
			tcp_free(conn);
			return 0;
		case TCP_SYN_RECEIVED:
			// TODO - if segments have been queued, wait for ESTABLISHED
			// before entering FIN-WAIT-1
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			return 0;
		case TCP_ESTABLISHED:
			// TODO - queue FIN after any outstanding segments
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			return 0;
		case TCP_CLOSE_WAIT:
			// queue FIN and state transition after sends
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_LAST_ACK);
			return 0;
		default:
			// connection error, already closing
			return TCP_ERROR_ALREADY_CLOSING;
	}
}

int send(int socket, const void* buffer, uint32_t length)
{
	dprintf("send(): %d\n", length);
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		dprintf("send(): invalid socket %d\n", socket);
		return TCP_ERROR_INVALID_SOCKET;
	} else if (conn->state != TCP_ESTABLISHED) {
		dprintf("send(): not connected %d\n", socket);
		return TCP_ERROR_NOT_CONNECTED;
	}
	dprintf("send(): wait spinlock\n");
	while (conn->send_buffer_spinlock);
	conn->send_buffer_spinlock++;
	dprintf("send(): buffer resize up\n");
	conn->send_buffer = krealloc(conn->send_buffer, length + conn->send_buffer_len);
	memcpy(conn->send_buffer + conn->send_buffer_len, buffer, length);
	conn->send_buffer_len += length;
	conn->send_buffer_spinlock--;
	dprintf("send(): done\n");
	return (int)length;
}

int connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port, bool blocking)
{
	int result = tcp_connect(target_addr, target_port, source_port);
	if (!blocking || result < 0) {
		if (result < 0) {
			dprintf("connect(): tcp_connect() returned error: %s\n", socket_error(result));
		}
		return result;
	}
	dprintf("connect(): tcp_connect() gave us fd %d\n", result);
	tcp_conn_t* conn = tcp_find_by_fd(result);
	time_t start = time(NULL);
	dprintf("Connect waiting: ");
	while (conn && conn->state < TCP_ESTABLISHED) {
		dprintf(".");
		asm volatile("hlt");
		if (time(NULL) - start > 10) {
			return TCP_ERROR_CONNECTION_FAILED;
		}
	};
	dprintf("\nconnect(): socket state ESTABLISHED\n");
	return conn->state == TCP_ESTABLISHED ? result : TCP_ERROR_CONNECTION_FAILED;
}

int closesocket(int socket)
{
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		return TCP_ERROR_INVALID_SOCKET;
	}
	tcp_free_fd(socket);
	return tcp_close(conn);
}

int recv(int socket, void* buffer, uint32_t maxlen, bool blocking, uint32_t timeout)
{
	dprintf("recv(): socket=%d blocking=%d\n", socket, blocking);
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		dprintf("recv(): invalid socket\n");
		return TCP_ERROR_INVALID_SOCKET;
	} else if (conn->state != TCP_ESTABLISHED) {
		dprintf("recv(): not connected\n");
		return TCP_ERROR_NOT_CONNECTED;
	}

	if (blocking) {
		dprintf("recv(): start blocking wait\n");
		time_t now = time(NULL);
		while (conn->recv_buffer_len == 0 || conn->recv_buffer == NULL) {
			if (time(NULL) - now > timeout || conn->state != TCP_ESTABLISHED) {
				return TCP_ERROR_CONNECTION_FAILED;
			}
		}
	}
	if (conn->recv_buffer_len > 0 && conn->recv_buffer != NULL) {
		dprintf("recv buffer spinlock\n");
		while (conn->recv_buffer_spinlock);
		conn->recv_buffer_spinlock++;
		/* There is buffered data to receive  */
		dprintf("recv buffer resize down\n");
		size_t amount_to_recv = conn->recv_buffer_len > maxlen ? maxlen : conn->recv_buffer_len;
		memcpy(buffer, conn->recv_buffer, amount_to_recv);
		/* Resize recv buffer down */
		if (conn->recv_buffer_len - amount_to_recv <= 0) {
			kfree(conn->recv_buffer);
			conn->recv_buffer = NULL;
		} else {
			conn->recv_buffer = krealloc(conn->recv_buffer + amount_to_recv, conn->recv_buffer_len - amount_to_recv);
		}
		conn->recv_buffer_len -= amount_to_recv;
		conn->recv_buffer_spinlock--;
		return amount_to_recv;
	}
	return 0;
}