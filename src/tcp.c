#include <kernel.h>

struct hashmap* tcb = NULL;

uint32_t get_isn()
{
	return (time(NULL) * 1000) - (get_ticks() % 1000);
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
uint64_t tcp_conn_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const tcp_conn_t* fa = item;
	return ((uint64_t)fa->local_addr << 32) + ((uint64_t)fa->remote_addr);
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

	//dump_hex((unsigned char*)pseudo, array_size);

	// Treat the packet header as a 2-byte-integer array
	// Sum all integers switch to network byte order
	uint16_t * array = (uint16_t*)pseudo;
	uint32_t sum = 0;
	for(int i = 0; i < array_size / 2; i++) {
		sum += htons(array[i]);
	}
	kprintf("\n");
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
void tcp_dump_segment(bool in, const ip_packet_t* encap_packet, const tcp_segment_t* segment, const tcp_options_t* options, size_t len, uint16_t our_checksum)
{
	char source_ip[15] = { 0 }, dest_ip[15] = { 0 };
	setforeground(current_console, our_checksum == segment->checksum ? COLOUR_LIGHTGREEN : COLOUR_LIGHTRED);
	get_ip_str(source_ip, encap_packet->src_ip);
	get_ip_str(dest_ip, encap_packet->dst_ip);
	kprintf(
		"TCP %s: %s:%d->%s:%d len=%ld seq=%d ack=%d off=%d\n\
flags[fin=%d,syn=%d,rst=%d,psh=%d,ack=%d,urg=%d,ece=%d,cwr=%d]\n\
window_size=%d,checksum=0x%04x (ours=0x%04x), urgent=%d options[mss=%d (%04x)]\n\n",
		in ? "IN" : "OUT",
		source_ip,
		segment->src_port,
		dest_ip,
		segment->dst_port,
		len,
		segment->seq,
		segment->ack,
		segment->flags.off,
		segment->flags.fin,
		segment->flags.syn,
		segment->flags.rst,
		segment->flags.psh,
		segment->flags.ack,
		segment->flags.urg,
		segment->flags.ece,
		segment->flags.cwr,
		segment->window_size,
		segment->checksum,
		our_checksum,
		segment->urgent,
		options->mss,
		options->mss
	);
	setforeground(current_console, COLOUR_WHITE);
	//dump_hex((unsigned char*)segment, len);
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

tcp_conn_t* tcp_find(uint32_t source_addr, uint32_t dest_addr, uint16_t source_port, uint16_t dest_port)
{
	if (tcb == NULL) {
		return NULL;
	}
	kprintf("tcp_find(%08x, %08x, %d, %d)\n", source_addr, dest_addr, source_port, dest_port);
	tcp_conn_t find_conn = { .local_addr = source_addr, .remote_addr = dest_addr, .local_port = source_port, .remote_port = dest_port };
	return (tcp_conn_t*)hashmap_get(tcb, &find_conn);
}

tcp_conn_t* tcp_set_state(tcp_conn_t* conn, tcp_state_t new_state)
{
	if (conn == NULL) {
		return NULL;
	}
	kprintf("tcp_set_state(%d)\n", new_state);
	conn->state = new_state;
	return conn;
}

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

tcp_conn_t* tcp_send_segment(tcp_conn_t *conn, uint32_t seq, uint8_t flags, const void *data, size_t count)
{
	if (conn == NULL) {
		return NULL;
	}

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
	packet->flags.off = (flags & TCP_SYN) ? 6 : 5;
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
	ip_send_packet(encap.dst_ip, packet, length, PROTOCOL_TCP);

	conn->snd_nxt += count;
	if (flags & (TCP_SYN | TCP_FIN)) {
		++conn->snd_nxt;
	}

	tcp_byte_order_in(packet);
	tcp_dump_segment(false, &encap, packet, &options, length, packet->checksum);

	return conn;
}



bool tcp_state_listen(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

bool tcp_state_syn_sent(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	kprintf("SYN_SENT state\n");

	if (segment->flags.ack) {
		if (seq_lte(segment->ack, conn->iss) || seq_gt(segment->ack, conn->snd_nxt)) {
			if (!segment->flags.rst) {
				tcp_send_segment(conn, segment->ack, TCP_RST, NULL, 0);
			}
		}
	}

	if (segment->flags.rst) {
		if (segment->flags.ack) {
			// Connection reset
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
			tcp_send_segment(conn, conn->snd_nxt, TCP_ACK, NULL, 0);

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

bool tcp_state_syn_received(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

bool tcp_handle_data_in(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	// insert packet into ordered list
	// check ordered list is complete, if it is deliver queued data to client
	// acknowlege receipt
	dump_hex(segment->payload, len - sizeof(tcp_segment_t));
	tcp_send_segment(conn, conn->snd_nxt, TCP_ACK, NULL, 0);
}

bool tcp_state_established(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_handle_data_in(encap_packet, segment, conn, options, len);
	return true;
}

bool tcp_state_fin_wait_1(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_handle_data_in(encap_packet, segment, conn, options, len);
	return true;
}

bool tcp_state_fin_wait_2(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_handle_data_in(encap_packet, segment, conn, options, len);
	return true;
}

bool tcp_state_close_wait(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	tcp_send_segment(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
        tcp_set_state(conn, TCP_LAST_ACK);
	return true;
}

bool tcp_state_closing(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

bool tcp_state_last_ack(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

bool tcp_state_time_wait(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	return true;
}

bool tcp_state_machine(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_conn_t* conn, const tcp_options_t* options, size_t len)
{
	kprintf("state machine, state=%d\n", conn->state);
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

void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len)
{
	tcp_options_t options;
	uint16_t our_checksum = tcp_calculate_checksum(encap_packet, segment, len);
	tcp_byte_order_in(segment);
	tcp_parse_options(segment, &options);
	tcp_dump_segment(true, encap_packet, segment, &options, len, our_checksum);
	if (our_checksum == segment->checksum) {
		tcp_conn_t* conn = tcp_find(*((uint32_t*)(&encap_packet->dst_ip)), *((uint32_t*)(&encap_packet->src_ip)), segment->dst_port, segment->src_port);
		if (conn) {
			tcp_state_machine(encap_packet, segment, conn, &options, len);
		}
	}
}

void tcp_init()
{
	tcb = hashmap_new(sizeof(tcp_conn_t), 0, 6, 28, tcp_conn_hash, tcp_conn_compare, NULL, NULL);
}

tcp_conn_t* tcp_connect(uint32_t target_addr, uint16_t target_port)
{
	tcp_conn_t conn;
	uint32_t isn = get_isn();
	unsigned char ip[4] = { 0 };

	gethostaddr(ip);

	conn.remote_addr = target_addr;
	conn.remote_port = target_port;
	conn.local_addr = *((uint32_t*)&ip);
	conn.local_port = 1025; // XXX FIXME
	conn.snd_una = isn;
	conn.snd_nxt = isn;
	conn.iss = isn;
	conn.snd_wnd = TCP_WINDOW_SIZE;
	conn.snd_up = 0;
	conn.snd_wl1 = 0;
	conn.snd_wl2 = 0;
	conn.rcv_nxt = 0;
	conn.rcv_wnd = TCP_WINDOW_SIZE;
	conn.rcv_up = 0;
	conn.irs = 0;

	tcp_send_segment(&conn, conn.snd_nxt, TCP_SYN, NULL, 0);
	tcp_set_state(&conn, TCP_SYN_SENT);

	hashmap_set(tcb, &conn);

	return tcp_find(conn.local_addr, conn.remote_addr, conn.local_port, conn.remote_port);
}