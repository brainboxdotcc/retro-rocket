#include <kernel.h>

static struct hashmap* tcb = NULL;

static uint32_t isn_rand_base = 0;
static uint32_t isn_tick_base = 0;
static int isn_init = 0;
static spinlock_t lock = 0;

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
	"Socket not listening",
	"Accept would block",
	"Connection timed out",
};

bool tcp_state_receive_fin(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len);
bool tcp_handle_data_in(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len);

const char* socket_error(int error_code) {
	if (error_code <= TCP_LAST_ERROR || error_code >= 0) {
		return "No error";
	}
	error_code = abs(error_code) - 1;
	return error_messages[error_code];
}

uint32_t get_isn() {
	if (!isn_init) {
		isn_rand_base = (uint32_t)(mt_rand() & 0xffffffffULL);
		isn_tick_base = get_ticks();
		isn_init = 1;
	}

	// delta since init
	uint32_t delta = (get_ticks() - isn_tick_base) * 250; // 250k/sec

	return isn_rand_base + delta;
}



bool seq_lt(uint32_t x, uint32_t y) {
	return ((int32_t)(x - y) < 0);
}

bool seq_lte(uint32_t x, uint32_t y) {
	return ((int32_t)(x - y) <= 0);
}

bool seq_gt(uint32_t x, uint32_t y) {
	return ((int32_t)(x - y) > 0);
}

bool seq_gte(uint32_t x, uint32_t y) {
	return ((int32_t)(x - y) >= 0);
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
	const tcp_conn_t *fa = a, *fb = b;

	/* Wildcard ONLY when both are listeners; respects bind addr + port. */
	if (fa->state == TCP_LISTEN && fb->state == TCP_LISTEN) {
		if (fa->local_port != fb->local_port) {
			return 1;
		}
		if (fa->local_addr != 0 && fb->local_addr != 0 && fa->local_addr != fb->local_addr) {
			return 1;
		}
		return 0;
	}

	/* Otherwise strict 4-tuple equality. */
	return (fa->local_addr == fb->local_addr &&
		fa->local_port == fb->local_port &&
		fa->remote_addr == fb->remote_addr &&
		fa->remote_port == fb->remote_port) ? 0 : 1;
}

/**
 * @brief Hash an ip connection
 * 
 * @param item item to hash
 * @param seed0 first seed from hashmap
 * @param seed1 second seed from hashmap
 * @return uint64_t hash bucket value
 */
uint64_t tcp_conn_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const tcp_conn_t *c = item;

	if (c->state == TCP_LISTEN) { /* no remote wildcarding here */
		uint64_t w0 = ((uint64_t)c->local_addr << 32);
		uint64_t w1 = ((uint64_t)c->local_port << 48);
		uint64_t buf[2] = { w0, w1 };
		return hashmap_sip(buf, sizeof buf, seed0, seed1);
	}

	uint64_t w0 = ((uint64_t)c->local_addr << 32) | (uint64_t)c->remote_addr;
	uint64_t w1 = ((uint64_t)c->local_port << 48) | ((uint64_t)c->remote_port << 32);
	uint64_t buf[2] = { w0, w1 };
	return hashmap_sip(buf, sizeof buf, seed0, seed1);
}

void tcp_free(tcp_conn_t* conn, bool with_lock)
{
	uint64_t flags;

	if (with_lock) {
		lock_spinlock_irq(&lock, &flags);
	}
	// Delete any outstanding segments
	for (tcp_ordered_list_t* t = conn->segment_list; t; ) {
		tcp_ordered_list_t* next = t->next;
		kfree_null(&t->segment);
		kfree_null(&t);
		t = next;
	}
	conn->segment_list = NULL;

	// Free pending queue if it exists
	if (conn->pending) {
		queue_free(conn->pending);
		conn->pending = NULL;
	}
	conn->backlog = 0;

	// Remove the TCB
	hashmap_delete(tcb, conn);
	if (with_lock) {
		unlock_spinlock_irq(&lock, flags);
	}
}

uint16_t tcp_calculate_checksum(ip_packet_t* packet, tcp_segment_t* segment, size_t len)
{
	int array_size = len + sizeof(tcp_ip_pseudo_header_t);
	tcp_ip_pseudo_header_t pseudo[array_size * 2];
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

	add_random_entropy(sum ^ (uint64_t)pseudo);
	return ~sum;
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
tcp_conn_t* tcp_find(uint32_t source_addr, uint32_t dest_addr, uint16_t source_port, uint16_t dest_port) {
	if (!tcb) {
		dprintf("TCB is null\n");
		return NULL;
	}

	/* 1) Exact 4-tuple: prefer established/half-open children */
	tcp_conn_t key = {
		.local_addr  = source_addr,
		.remote_addr = dest_addr,
		.local_port  = source_port,
		.remote_port = dest_port,
		.state       = TCP_SYN_RECEIVED    /* any non-LISTEN value works */
	};
	tcp_conn_t *c = hashmap_get(tcb, &key);
	if (c) {
		return c;
	}

	/* 2) Fallback to listener on this bind address/port */
	key.remote_addr = 0;
	key.remote_port = 0;
	key.state = TCP_LISTEN;
	return hashmap_get(tcb, &key);
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

	ip_packet_t encap;
	tcp_options_t options = { .mss = flags & TCP_SYN ? 1460 : 0 };
	uint16_t length = sizeof(tcp_segment_t) + count + (flags & TCP_SYN ? 4 : 0);
	tcp_segment_t packet[length];
	memset(&packet, 0, length);
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

#ifdef TCP_TRACE
	tcp_byte_order_in(packet);
	tcp_dump_segment(false, conn, &encap, packet, &options, length, packet->checksum);
	tcp_byte_order_out(packet);
#endif

	ip_send_packet(encap.dst_ip, packet, length, PROTOCOL_TCP);

	conn->snd_nxt += count;
	if (flags & (TCP_SYN | TCP_FIN)) {
		++conn->snd_nxt;
	}

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
	dprintf("tcp_write %lu bytes\n", count);
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
	tcp_ordered_list_t *cur = conn->segment_list;
	tcp_ordered_list_t *next_ord = NULL;

	// find insertion point
	while (cur && seq_lt(cur->segment->seq, segment->seq)) {
		cur = cur->next;
	}

	// check overlap with previous
	if (cur && cur->prev) {
		tcp_segment_t *prev = cur->prev->segment;
		size_t prev_end = prev->seq + cur->prev->len;

		if (segment->seq == prev->seq && len == cur->prev->len) {
			// exact duplicate -> drop
			return NULL;
		}

		if (seq_gt(prev_end, segment->seq)) {
			// overlap: shrink the previous segment so this one overwrites
			cur->prev->len = segment->seq - prev->seq;
		}
	}

	// remove any existing segments fully covered by the new one
	while (cur) {
		size_t new_end = segment->seq + len;
		size_t cur_end = cur->segment->seq + cur->len;

		if (seq_gte(new_end, cur_end)) {
			// new completely covers cur -> remove cur
			next_ord = cur->next;
			if (cur->prev) cur->prev->next = cur->next;
			if (cur->next) cur->next->prev = cur->prev;
			if (conn->segment_list == cur) conn->segment_list = next_ord;
			kfree_null(&cur->segment);
			kfree_null(&cur);
			cur = next_ord;
			continue;
		}

		if (seq_gt(new_end, cur->segment->seq)) {
			// partial overlap -> shrink cur from the left
			size_t overlap = new_end - cur->segment->seq;
			cur->segment->seq += overlap;
			cur->len -= overlap;
		}

		break; // no further overlaps possible
	}

	// allocate new node
	tcp_ordered_list_t *new = kmalloc(sizeof(tcp_ordered_list_t));
	if (!new) {
		return NULL;
	}
	new->segment = kmalloc(len + tcp_header_size(segment));
	if (!new->segment) {
		kfree_null(&new);
		return NULL;
	}
	memcpy(new->segment, segment, len + tcp_header_size(segment));
	new->len = len;
	new->prev = NULL;
	new->next = NULL;

	// insert before cur
	if (cur) {
		new->next = cur;
		new->prev = cur->prev;
		cur->prev = new;
		if (new->prev) {
			new->prev->next = new;
		} else {
			conn->segment_list = new;
		}
	} else {
		// end of list
		if (conn->segment_list) {
			tcp_ordered_list_t *tail = conn->segment_list;
			while (tail->next) tail = tail->next;
			tail->next = new;
			new->prev = tail;
		} else {
			conn->segment_list = new;
		}
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
	tcp_ordered_list_t* cur = conn->segment_list;

	while (cur) {
		// If this segment isn't the next expected, stop
		if (conn->rcv_nxt != cur->segment->seq) {
			break;
		}

		// Adjust for options
		uint8_t* payload = cur->segment->payload;
		size_t payload_len = cur->len;
		if (cur->segment->flags.off > TCP_PACKET_SIZE_OFF) {
			size_t options_len = (cur->segment->flags.off - TCP_PACKET_SIZE_OFF) * 4;
			if (options_len < payload_len) {
				payload += options_len;
				payload_len -= options_len;
			} else {
				payload_len = 0; // corrupt options length safeguard
			}
		}

		// increment what we have received in our connection state
		conn->rcv_nxt += payload_len;

		// Append to recv buffer
		if (payload_len > 0) {
			dprintf("Resize recv buffer %p from %lu to %lu\n", conn->recv_buffer, conn->recv_buffer_len, conn->recv_buffer_len + payload_len);
			conn->recv_buffer = krealloc(conn->recv_buffer, conn->recv_buffer_len + payload_len);
			memcpy(conn->recv_buffer + conn->recv_buffer_len, payload, payload_len);
			conn->recv_buffer_len += payload_len;
		}

		// Save next before freeing
		tcp_ordered_list_t* next = cur->next;

		// Remove from doubly linked list
		if (cur->prev) {
			cur->prev->next = cur->next;
		}
		if (cur->next) {
			cur->next->prev = cur->prev;
		}
		if (cur == conn->segment_list) {
			conn->segment_list = cur->next;
		}

		kfree_null(&cur->segment);
		kfree_null(&cur);

		// Advance
		cur = next;
	}
}

bool tcp_state_listen(ip_packet_t *encap_packet, tcp_segment_t *segment, tcp_conn_t *listener, const tcp_options_t *options, size_t len) {
	/* Only accept a bare SYN in LISTEN. Drop others. */
	if (!segment->flags.syn || segment->flags.ack || segment->flags.fin || segment->flags.rst) {
		dprintf("Invalid listen state\n");
		return true;
	}

	/* Build a child PCB from the incoming SYN. */
	tcp_conn_t child;
	memset(&child, 0, sizeof(child));

	child.local_addr  = *((uint32_t *)&encap_packet->dst_ip);
	child.local_port  = segment->dst_port;
	child.remote_addr = *((uint32_t *)&encap_packet->src_ip);
	child.remote_port = segment->src_port;

	dprintf("Allocated child remote addr %08x\n", child.remote_addr);

	/* Initial sequencing: passive open. */
	child.iss     = get_isn();
	child.snd_una = child.iss;
	child.snd_nxt = child.iss;          /* tcp_send_segment will advance by 1 for SYN */
	child.snd_lst = 0;

	child.irs     = segment->seq;
	child.rcv_nxt = segment->seq + 1;
	child.rcv_lst = 0;

	child.snd_wnd = TCP_WINDOW_SIZE;
	child.rcv_wnd = TCP_WINDOW_SIZE;

	child.fd                    = -1;
	child.segment_list          = NULL;
	child.recv_eof_pos          = -1;
	child.send_eof_pos          = -1;
	child.recv_buffer           = NULL;
	child.send_buffer           = NULL;
	child.recv_buffer_len       = 0;
	child.send_buffer_len       = 0;
	child.recv_buffer_spinlock  = 0;
	child.send_buffer_spinlock  = 0;
	child.pending               = NULL;
	child.backlog               = 0;

	tcp_set_state(&child, TCP_SYN_RECEIVED);

	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);

	/* Insert child keyed by full 4-tuple, then retrieve the stored copy. */
	hashmap_set(tcb, &child);

	/* Fetch the stored child EXACTLY (avoid LISTEN wildcard on the temp key). */
	tcp_conn_t find_key = {
		.local_addr  = child.local_addr,
		.remote_addr = child.remote_addr,
		.local_port  = child.local_port,
		.remote_port = child.remote_port,
		.state       = TCP_SYN_RECEIVED   /* anything non-LISTEN works */
	};
	tcp_conn_t *new_conn = hashmap_get(tcb, &find_key);
	if (!new_conn) {
		unlock_spinlock_irq(&lock, flags);
		dprintf("Cant find newly created child connection\n");
		return true;
	}

	/* Queue and send from the CHILD */
	if (listener->pending) {
		queue_push(listener->pending, new_conn);
	}
	dprintf("LISTEN send pcb=%p state=%d %08x:%u -> %08x:%u\n", new_conn, new_conn->state, new_conn->local_addr, new_conn->local_port, new_conn->remote_addr, new_conn->remote_port);
	tcp_send_segment(new_conn, new_conn->snd_nxt, TCP_SYN | TCP_ACK, NULL, 0);


	unlock_spinlock_irq(&lock, flags);
	dprintf("New client accepted\n");
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

bool tcp_send_fin_ack(tcp_conn_t*conn)
{
	/* Check for duplicate ACKs (same rcv_nxt as the previous) and drop them */
	if (conn->rcv_nxt == conn->rcv_lst) {
		return true;
	}
	conn->snd_lst = conn->snd_nxt;
	conn->rcv_lst = conn->rcv_nxt;
	return tcp_send_segment(conn, conn->snd_nxt, TCP_ACK | TCP_FIN, NULL, 0);
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
bool tcp_state_syn_sent(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len) {
	dprintf("SYN_SENT: iss=%u snd_nxt=%u seg.ack=%u\n", conn->iss, conn->snd_nxt, segment->ack);

	if (segment->flags.ack) {
		// RFC 793: acceptable if ISS < SEG.ACK <= SND.NXT
		if (!(seq_gt(segment->ack, conn->iss) && seq_lte(segment->ack, conn->snd_nxt + 1))) {
			if (!segment->flags.rst) {
				dprintf(
					"rejecting with RST: iss=%u snd_nxt=%u seg.ack=%u, "
					"seq_gt(seg.ack, iss)=%d seq_lte(seg.ack, snd_nxt)=%d\n",
					conn->iss, conn->snd_nxt, segment->ack,
					seq_gt(segment->ack, conn->iss),
					seq_lte(segment->ack, conn->snd_nxt + 1)
				);
				tcp_send_segment(conn, segment->ack, TCP_RST, NULL, 0);
			}
			dprintf("unacceptable ACK, dropping connection: iss=%u snd_nxt=%u seg.ack=%u\n", conn->iss, conn->snd_nxt, segment->ack);
			return true; // unacceptable ACK, drop/abort
		}
	}


	if (segment->flags.rst) {
		if (segment->flags.ack) {
			// TODO: Connection reset (propagate upwards)
			dprintf("TODO: Connection reset, propagate into conn_t\n");
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
bool tcp_state_syn_received(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len) {
	if (segment->flags.fin) {
		return tcp_state_receive_fin(encap_packet, segment, conn, options, len);
	}

	if (segment->flags.rst) {
		dprintf("*** TCP *** RST in SYN-RECEIVED\n");
		tcp_free(conn, true);
		return false;
	}

	if (segment->flags.ack) {
		/* Acceptable if our SYN was acked: ISS < SEG.ACK <= SND.NXT */
		if (seq_gt(segment->ack, conn->iss) && seq_lte(segment->ack, conn->snd_nxt + 1)) {
			conn->snd_una = segment->ack;
			conn->snd_wnd = segment->window_size;
			conn->snd_wl1 = segment->seq;
			conn->snd_wl2 = segment->ack;

			tcp_set_state(conn, TCP_ESTABLISHED);

			/* If any payload, process; otherwise send an ACK (idempotent). */
			if (len - tcp_header_size(segment) > 0) {
				tcp_handle_data_in(encap_packet, segment, conn, options, len);
			} else {
				tcp_send_ack(conn);
			}
		} else {
			/* Unacceptable ACK: polite RST per RFC. */
			tcp_send_segment(conn, segment->ack, TCP_RST, NULL, 0);
		}
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
			tcp_free(conn, true);
			break;
		case TCP_ESTABLISHED:
		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
		case TCP_CLOSE_WAIT:
			// Connection reset by peer
			dprintf("*** TCP *** Connection reset by peer\n");
			tcp_free(conn, true);
			break;
		case TCP_CLOSING:
		case TCP_LAST_ACK:
		case TCP_TIME_WAIT:
			// Connection closed
			dprintf("*** TCP *** Connection gracefully closed\n");
			tcp_free(conn, true);
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
bool tcp_handle_data_in(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len) {
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
	// check ordered list is complete (no seq gaps), if it is delivered queued data to client
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
	// Did we get a FIN?
	if (segment->flags.fin) {
		conn->rcv_nxt = segment->seq + len - tcp_header_size(segment) + 1; // step over FIN
		tcp_send_fin_ack(conn);

		if (segment->flags.ack && seq_lte(conn->snd_una, segment->ack) && seq_lte(segment->ack, conn->snd_nxt)) {
			// our FIN was acknowledged
			tcp_set_state(conn, TCP_TIME_WAIT);
		} else {
			// simultaneous FIN
			tcp_set_state(conn, TCP_CLOSING);
		}
	}

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
			dprintf("state machine; listen\n");
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
	tcp_conn_t* conn = tcp_find(*((uint32_t*)(&encap_packet->dst_ip)), *((uint32_t*)(&encap_packet->src_ip)), segment->dst_port, segment->src_port);
	tcp_dump_segment(true, conn, encap_packet, segment, &options, len, our_checksum);
	if (!conn) {
		dprintf("Segment for unknown connection dropped\n");
		return;
	}
	if (our_checksum == segment->checksum) {
		tcp_state_machine(encap_packet, segment, conn, &options, len);
	} else {
		dprintf("tcp_handle_packet dropped packet due to invalid sum\n");
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
	uint64_t flags;

	lock_spinlock_irq(&lock, &flags);
	while (hashmap_iter(tcb, &iter, &item)) {
		tcp_conn_t *conn = item;
		if (conn && conn->state == TCP_ESTABLISHED) {
			if (conn->send_buffer_len > 0 && conn->send_buffer != NULL) {
				dprintf("socket has %lu to send\n", conn->send_buffer_len);
				/* There is buffered data to send from high level functions */
				size_t amount_to_send = conn->send_buffer_len > 1460 ? 1460 : conn->send_buffer_len;
				if (tcp_write(conn, conn->send_buffer, amount_to_send) < 0) {
					dprintf("tcp_write returned error\n");
				}
				if (amount_to_send >= conn->send_buffer_len) {
					// Everything is sent, free buffer entirely
					kfree_null(&conn->send_buffer);
					conn->send_buffer = NULL;
					conn->send_buffer_len = 0;
				} else {
					size_t new_len = conn->send_buffer_len - amount_to_send;
					void *new_buf = kmalloc(new_len);
					if (new_buf) {
						memcpy(new_buf, (uint8_t *)conn->send_buffer + amount_to_send, new_len);
					}
					kfree_null(&conn->send_buffer);
					conn->send_buffer = new_buf;
					conn->send_buffer_len = new_len;
				}
			}
		} else if (conn && conn->state == TCP_TIME_WAIT && seq_gte(get_isn(), conn->msl_time)) {
			tcp_free(conn, false);
			break;
		}
	}
	unlock_spinlock_irq(&lock, flags);
}

/**
 * @brief Initialise TCP
 */
void tcp_init()
{
	tcb = hashmap_new(sizeof(tcp_conn_t), 0, 6, 28, tcp_conn_hash, tcp_conn_compare, NULL, NULL);
	proc_register_idle(tcp_idle, IDLE_FOREGROUND, 1);
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
	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);

	void *item;
	size_t iter = 0;
	while (hashmap_iter(tcb, &iter, &item)) {
		const tcp_conn_t *conn = item;
		if (conn->local_addr == addr && ((type == TCP_PORT_LOCAL && conn->local_port == port) || (type == TCP_PORT_REMOTE && conn->remote_port == port))) {
			unlock_spinlock_irq(&lock, flags);
			return true;
		}
	}
	unlock_spinlock_irq(&lock, flags);
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
			dprintf("tcp_alloc_port, no local ports available\n");
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
	dprintf("connect: allocated local port: %u local addr %08x\n", conn.local_port, conn.local_addr);
	conn.snd_una = isn;
	conn.snd_nxt = isn + 1;
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
	conn.pending = NULL;
	conn.backlog = 0;

	tcp_set_state(&conn, TCP_SYN_SENT);

	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);
	hashmap_set(tcb, &conn);

	tcp_conn_t* new_conn = tcp_find(conn.local_addr, conn.remote_addr, conn.local_port, conn.remote_port);
	if (!new_conn) {
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_OUT_OF_MEMORY;
	}

	new_conn->fd = tcp_allocate_fd(new_conn);
	if (new_conn->fd == -1) {
		dprintf("tcp_connect() allocation of fd failed\n");
		tcp_free(new_conn, false);
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	tcp_send_segment(new_conn, new_conn->snd_nxt, TCP_SYN, NULL, 0);
	unlock_spinlock_irq(&lock, flags);
	dprintf("tcp_connect() done with fd %u\n", new_conn->fd);
	return new_conn->fd;
}

int tcp_close(tcp_conn_t* conn)
{
	dprintf("tcp_close()\n");
	if (conn == NULL) {
		return TCP_ERROR_INVALID_CONNECTION;
	}
	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);
	dprintf("tcp_close() - spin locked\n");
	switch (conn->state) {
		case TCP_LISTEN:
		case TCP_SYN_SENT:
			dprintf("tcp_close() - LISTEN or SYN_SENT\n");
			tcp_free(conn, false);
			unlock_spinlock_irq(&lock, flags);
			return 0;
		case TCP_SYN_RECEIVED:
			// TODO - if segments have been queued, wait for ESTABLISHED
			// before entering FIN-WAIT-1
			dprintf("tcp_close() - SYN_RECV\n");
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			unlock_spinlock_irq(&lock, flags);
			return 0;
		case TCP_ESTABLISHED:
			// TODO - queue FIN after any outstanding segments
			dprintf("tcp_close() - ESTABLISHED\n");
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			unlock_spinlock_irq(&lock, flags);
			return 0;
		case TCP_CLOSE_WAIT:
			// queue FIN and state transition after sends
			dprintf("tcp_close() - CLOSE_WAIT\n");
			tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, NULL, 0);
			tcp_set_state(conn, TCP_LAST_ACK);
			unlock_spinlock_irq(&lock, flags);
			return 0;
		default:
			dprintf("tcp_close() - DEFAULT\n");
			// connection error, already closing
			unlock_spinlock_irq(&lock, flags);
			return TCP_ERROR_ALREADY_CLOSING;
	}
}

int send(int socket, const void* buffer, uint32_t length)
{
	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		dprintf("send(): invalid socket %d\n", socket);
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_INVALID_SOCKET;
	} else if (conn->state != TCP_ESTABLISHED) {
		dprintf("send(): not connected %d\n", socket);
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_NOT_CONNECTED;
	}
	conn->send_buffer = krealloc(conn->send_buffer, length + conn->send_buffer_len);
	memcpy(conn->send_buffer + conn->send_buffer_len, buffer, length);
	conn->send_buffer_len += length;
	unlock_spinlock_irq(&lock, flags);
	dprintf("sockwrite wrote length %u\n", length);
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
	tcp_conn_t* conn = tcp_find_by_fd(result);
	time_t start = get_ticks();
	while (conn && conn->state < TCP_ESTABLISHED) {
		__asm__ volatile("hlt");
		if (get_ticks() - start > 3000) {
			return TCP_CONNECTION_TIMED_OUT;
		}
	};
	dprintf("connect() result: %u\n", conn && conn->state == TCP_ESTABLISHED ? result : TCP_ERROR_CONNECTION_FAILED);
	return conn && conn->state == TCP_ESTABLISHED ? result : TCP_ERROR_CONNECTION_FAILED;
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

bool is_connected(int socket)
{
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		return false;
	} else if (conn->state != TCP_ESTABLISHED) {
		return false;
	}
	return conn->state == TCP_ESTABLISHED;
}

int recv(int socket, void* buffer, uint32_t maxlen, bool blocking, uint32_t timeout)
{
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (conn == NULL) {
		dprintf("recv(): invalid socket\n");
		return TCP_ERROR_INVALID_SOCKET;
	} else if (conn->state != TCP_ESTABLISHED) {
		dprintf("recv(): not connected\n");
		return TCP_ERROR_NOT_CONNECTED;
	}

	if (blocking) {
		time_t now = get_ticks();
		while (conn->recv_buffer_len == 0 || conn->recv_buffer == NULL) {
			if (get_ticks() - now > timeout || conn->state != TCP_ESTABLISHED) {
				return TCP_ERROR_CONNECTION_FAILED;
			}
		}
	}
	if (conn->recv_buffer_len > 0 && conn->recv_buffer != NULL) {
		uint64_t flags;
		lock_spinlock_irq(&lock, &flags);
		/* There is buffered data to receive  */
		size_t amount_to_recv = conn->recv_buffer_len > maxlen ? maxlen : conn->recv_buffer_len;
		memcpy(buffer, conn->recv_buffer, amount_to_recv);
		/* Resize recv buffer down */
		if (conn->recv_buffer_len - amount_to_recv <= 0) {
			kfree_null(&conn->recv_buffer);
			conn->recv_buffer_len = 0;
		} else {
			size_t new_len = conn->recv_buffer_len - amount_to_recv;
			void *new_buf = kmalloc(new_len);
			if (new_buf) {
				memcpy(new_buf, conn->recv_buffer + amount_to_recv, new_len);
			}
			kfree_null(&conn->recv_buffer);
			conn->recv_buffer = new_buf;
			conn->recv_buffer_len = new_len;
		}
		unlock_spinlock_irq(&lock, flags);
		return amount_to_recv;
	}
	return 0;
}

bool sock_ready_to_read(int socket) {
	tcp_conn_t* conn = tcp_find_by_fd(socket);
	if (!conn || conn->state != TCP_ESTABLISHED) {
		return false;
	}
	return conn->recv_buffer && conn->recv_buffer_len > 0;
}


int tcp_listen(uint32_t addr, uint16_t port, int backlog)
{
	if (tcp_port_in_use(addr, port, TCP_PORT_LOCAL)) {
		dprintf("listen: Port in use\n");
		return TCP_ERROR_PORT_IN_USE;
	}

	tcp_conn_t conn;
	memset(&conn, 0, sizeof(conn));

	unsigned char ip[4] = { 0 };
	gethostaddr(ip);

	conn.local_addr = *((uint32_t*)&ip);
	conn.local_port = tcp_alloc_port(conn.local_addr, port, TCP_PORT_LOCAL);
	conn.remote_addr = 0;
	conn.remote_port = 0;

	tcp_set_state(&conn, TCP_LISTEN);

	conn.backlog = backlog;
	conn.pending = queue_new();
	if (!conn.pending) {
		dprintf("listen: Out of memory\n");
		return TCP_ERROR_OUT_OF_MEMORY;
	}

	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);
	hashmap_set(tcb, &conn);

	tcp_conn_t* new_conn = tcp_find(conn.local_addr, conn.remote_addr, conn.local_port, conn.remote_port);
	if (!new_conn) {
		queue_free(conn.pending);
		unlock_spinlock_irq(&lock, flags);
		dprintf("listen: Out of memory (2)\n");
		return TCP_ERROR_OUT_OF_MEMORY;
	}

	new_conn->fd = tcp_allocate_fd(new_conn);
	if (new_conn->fd == -1) {
		queue_free(new_conn->pending);
		tcp_free(new_conn, false);
		unlock_spinlock_irq(&lock, flags);
		dprintf("listen: Out of descriptors\n");
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	unlock_spinlock_irq(&lock, flags);
	dprintf("listen: listening on %d\n", port);
	return new_conn->fd;
}

int tcp_accept(int socket) {
	tcp_conn_t *listener = tcp_find_by_fd(socket);
	if (!listener) {
		dprintf("accept: Invalid socket %d\n", socket);
		return TCP_ERROR_INVALID_SOCKET;
	}
	if (listener->state != TCP_LISTEN) {
		dprintf("accept: Not listening: %d\n", socket);
		return TCP_ERROR_NOT_LISTENING;
	}

	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);

	if (queue_empty(listener->pending)) {
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_WOULD_BLOCK;
	}

	/* Look at the oldest pending child without removing it. */
	tcp_conn_t *head = queue_peek(listener->pending);
	if (!head || head->state != TCP_ESTABLISHED) {
		/* Handshake not completed yet; keep strict FIFO by not popping. */
		unlock_spinlock_irq(&lock, flags);
		return TCP_ERROR_WOULD_BLOCK;
	}

	/* Now it’s established; remove it from the queue and hand it out. */
	tcp_conn_t *conn = queue_pop(listener->pending);
	if (!conn) {
		unlock_spinlock_irq(&lock, flags);
		dprintf("accept: Connection failed: %d\n", socket);
		return TCP_ERROR_CONNECTION_FAILED;
	}

	conn->fd = tcp_allocate_fd(conn);
	if (conn->fd == -1) {
		unlock_spinlock_irq(&lock, flags);
		dprintf("accept: Out of descriptors: %d\n", socket);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	unlock_spinlock_irq(&lock, flags);
	dprintf("accept: New inbound: %d\n", conn->fd);
	return conn->fd;
}

static inline bool tcp_tx_drained_nolock(const tcp_conn_t *c) {
	if (!c) {
		return true;
	} else if (c->state != TCP_ESTABLISHED) {
		return true;
	}
	return (c->send_buffer_len == 0 && c->send_buffer == NULL);
}

bool sock_sent(int fd) {
	uint64_t flags;
	lock_spinlock_irq(&lock, &flags);
	bool drained = tcp_tx_drained_nolock(tcp_find_by_fd(fd));
	unlock_spinlock_irq(&lock, flags);
	return drained;
}
