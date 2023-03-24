// https://github.com/pdoane/osdev/blob/master/net/tcp.c
#include <kernel.h>

static uint32_t base_isn = 0;
static linked_list_t free_connections = { &free_connections, &free_connections };
linked_list_t active_connections = { &active_connections, &active_connections};

const char *tcp_state_strings[] = {
	"CLOSED",
	"LISTEN",
	"SYN_SENT",
	"SYN_RECEIVED",
	"ESTABLISHED",
	"FIN_WAIT_1",
	"FIN_WAIT_2",
	"CLOSE_WAIT",
	"CLOSING",
	"LAST_ACK",
	"TIME_WAIT"
};


static void tcp_recv_fin(tcp_conn_t *conn, tcp_header_t *hdr);
static void tcp_recv_process(tcp_conn_t *conn);
static void tcp_recv_ack(tcp_conn_t *conn, tcp_header_t *hdr);
static void tcp_recv_syn(tcp_conn_t *conn, tcp_header_t *hdr);
static void tcp_recv_rst(tcp_conn_t *conn, tcp_header_t *hdr);
static void tcp_recv_syn_sent(tcp_conn_t *conn, tcp_header_t *hdr);
static void tcp_recv_closed(tcp_checksummed_t *phdr, tcp_header_t *hdr);

uint16_t net_checksum(const u8 *data, const u8 *end)
{
	uint16_t sum = net_checksum_accumulate(data, end, 0);
	return net_checksum_final(sum);
}

// ------------------------------------------------------------------------------------------------
uint16_t net_checksum_accumulate(const u8 *data, const u8 *end, uint sum)
{
	uint16_t len = end - data;
	uint16_t *p = (uint16_t *)data;

	while (len > 1)
	{
		sum += *p++;
		len -= 2;
	}

	if (len)
	{
		sum += *(u8 *)p;
	}

	return sum;
}

// ------------------------------------------------------------------------------------------------
uint16_t net_checksum_final(uint sum)
{
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);

	uint16_t temp = ~sum;
	return ((temp & 0x00ff) << 8) | ((temp & 0xff00) >> 8); // TODO - shouldn't swap this twice
}

// ------------------------------------------------------------------------------------------------
static bool tcp_parse_options(tcp_options_t *opt, const uint8_t *p, const uint8_t *end)
{
	memset(opt, 0, sizeof(*opt));

	while (p < end) {
		uint8_t type = *p++;

		if (type == TCP_OPT_NOP) {
			continue;
		} else if (type == TCP_OPT_END) {
			break;
		} else {
			uint8_t opt_len = *p++;

			if (opt_len < 2) {
				return false;
			}

			const uint8_t *next = p + opt_len - 2;
			if (next > end) {
				return false;
			}

			switch (type) {
				case TCP_OPT_MSS:
					opt->mss = ntohs(*(uint16_t *)p);
					break;
			}

			p = next;
		}
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
static void tcp_debug(const NetBuf *pkt)
{
	if (pkt->start + sizeof(tcp_header_t) > pkt->end) {
		return;
	}

	tcp_checksummed_t *phdr = (tcp_checksummed_t *)(pkt->start - sizeof(tcp_checksummed_t));

	const tcp_header_t *hdr = (const tcp_header_t *)pkt->start;

	uint16_t src_port = ntohs(hdr->src_port);
	uint16_t dst_port = ntohs(hdr->dst_port);
	uint32_t seq = ntohl(hdr->seq);
	uint32_t ack = ntohl(hdr->ack);
	uint16_t window_size = ntohs(hdr->window_size);
	uint16_t checksum = ntohs(hdr->checksum);
	uint16_t urgent = ntohs(hdr->urgent);

	uint16_t checksum2 = net_checksum(pkt->start - sizeof(tcp_checksummed_t), pkt->end);

	uint32_t hdrLen = hdr->off >> 2;
	//const uint8_t *data = (pkt->start + hdrLen);
	uint32_t data_len = (pkt->end - pkt->start) - hdrLen;

	kprintf("  TCP: src=%08x:%08x dst=%s:%d\n", &phdr->src, src_port, &phdr->dst, dst_port);
	kprintf("  TCP: seq=%u ack=%u data_len=%u\n", seq, ack, data_len);
	kprintf("  TCP: flags=%02x window=%u urgent=%u checksum=%u%c\n", hdr->flags, window_size, urgent, checksum, checksum2 ? '!' : ' ');

	if (hdrLen > sizeof(tcp_header_t)) {
		const uint8_t *p = pkt->start + sizeof(tcp_header_t);
		const uint8_t *end = p + hdrLen;

		tcp_opt_t opt;
		tcp_parse_options(&opt, p, end);

		if (opt.mss) {
			kprintf("  TCP: mss=%u\n", opt.mss);
		}
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_set_state(tcp_conn_t *conn, uint32_t state)
{
	uint32_t oldState = conn->state;
	conn->state = state;

	if (conn->on_state) {
		conn->on_state(conn, oldState, state);
	}
}

// ------------------------------------------------------------------------------------------------
static tcp_conn_t *tcp_alloc()
{
	linked_list_t *p = free_connections.next;
	if (p != &free_connections) {
		link_remove(p);
		return link_data(p, tcp_conn_t, link);
	} else {
		return VMAlloc(sizeof(tcp_conn_t));
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_free(tcp_conn_t *conn)
{
	if (conn->state != TCP_CLOSED) {
		tcp_set_state(conn, TCP_CLOSED);
	}

	NetBuf *pkt;
	NetBuf *next;
	list_for_each_safe(pkt, next, conn->resequence, link)
	{
		link_remove(&pkt->link);
		NetReleaseBuf(pkt);
	}

	link_move_before(&free_connections, &conn->link);
}

// ------------------------------------------------------------------------------------------------
static void tcp_send_packet(tcp_conn_t *conn, uint32_t seq, uint8_t flags, const void *data, uint32_t count)
{
	NetBuf *pkt = NetAllocBuf();

	// Header
	tcp_header_t *hdr = (tcp_header_t *)pkt->start;
	hdr->src_port = conn->local_port;
	hdr->dst_port = conn->remote_port;
	hdr->seq = seq;
	hdr->ack = flags & TCP_ACK ? conn->rcv_nxt : 0;
	hdr->off = 0;
	hdr->flags = flags;
	hdr->window_size = TCP_WINDOW_SIZE;
	hdr->checksum = 0;
	hdr->urgent = 0;

	hdr->src_port = htons(hdr->src_port);
	hdr->dst_port = htons(hdr->dst_port);
	hdr->seq = htonl(hdr->seq);
	hdr->ack = htonl(hdr->ack);
	hdr->window_size = htons(hdr->window_size);
	hdr->checksum = htons(hdr->checksum);
	hdr->urgent = htons(hdr->urgent);

	uint8_t *p = pkt->start + sizeof(tcp_header_t);

	if (flags & TCP_SYN) {
		// Maximum Segment Size
		p[0] = TCP_OPT_MSS;
		p[1] = 4;
		*(uint16_t *)(p + 2) = NetSwap16(1460);
		p += p[1];
	}

	// Option End
	while ((p - pkt->start) & 3) {
		*p++ = 0;
	}

	hdr->off = (p - pkt->start) << 2;

	// Data
	memcpy(p, data, count);
	pkt->end = p + count;

	// Pseudo Header
	tcp_checksummed_t *phdr = (tcp_checksummed_t *)(pkt->start - sizeof(tcp_checksummed_t));
	phdr->src = conn->local_addr;
	phdr->dst = conn->remote_addr;
	phdr->reserved = 0;
	phdr->protocol = PROTOCOL_TCP;
	phdr->len = NetSwap16(pkt->end - pkt->start);

	// Checksum
	uint16_t checksum = net_checksum(pkt->start - sizeof(tcp_checksummed_t), pkt->end);
	hdr->checksum = NetSwap16(checksum);

	// Transmit
	tcp_debug(pkt);
	ip_send_packet(conn->intf, conn->remote_addr, conn->remote_addr, IP_PROTOCOL_TCP, pkt);

	// Update State
	conn->snd_nxt += count;
	if (flags & (TCP_SYN | TCP_FIN)) {
		++conn->snd_nxt;
	}
}

// ------------------------------------------------------------------------------------------------
static tcp_conn_t *tcp_find(const uint32_t srcAddr, uint16_t src_port,
	const uint32_t dstAddr, uint16_t dst_port)
{
	tcp_conn_t *conn;
	list_for_each(conn, active_connections, link)
	{
		if (src_port == conn->remote_port && dst_port == conn->local_port && srcAddr == conn->remote_addr && dstAddr == conn->local_addr) {
			return conn;
		}
	}

	return 0;
}

// ------------------------------------------------------------------------------------------------
static void tcp_error(tcp_conn_t *conn, uint32_t error)
{
	if (conn->on_error) {
		conn->on_error(conn, error);
	}

	tcp_free(conn);
}

// ------------------------------------------------------------------------------------------------
void TcpInit()
{
	base_isn = time(NULL);
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_closed(tcp_checksummed_t *phdr, tcp_header_t *hdr)
{
	// Drop packet if this is a RST
	if (hdr->flags & TCP_RST) {
		return;
	}

	// Find an appropriate interface to route packet
	const uint32_t *dstAddr = &phdr->src;

	// Create dummy connection for sending RST
	tcp_conn_t rstConn;
	memset(&rstConn, 0, sizeof(tcp_conn_t));

	rstConn.intf = route->intf;
	rstConn.local_addr = phdr->dst;
	rstConn.local_port = hdr->dst_port;
	rstConn.remote_addr = phdr->src;
	rstConn.remote_port = hdr->src_port;

	if (hdr->flags & TCP_ACK) {
		tcp_send_packet(&rstConn, hdr->ack, TCP_RST, 0, 0);
	} else {
		uint32_t hdrLen = hdr->off >> 2;
		uint32_t data_len = phdr->len - hdrLen;

		rstConn.rcv_nxt = hdr->seq + data_len;

		tcp_send_packet(&rstConn, 0, TCP_RST | TCP_ACK, 0, 0);
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_syn_sent(tcp_conn_t *conn, tcp_header_t *hdr)
{
	uint32_t flags = hdr->flags;

	// Check for bad ACK first.
	if (flags & TCP_ACK) {
		if (SEQ_LE(hdr->ack, conn->iss) || SEQ_GT(hdr->ack, conn->snd_nxt)) {
			if (~flags & TCP_RST) {
				tcp_send_packet(conn, hdr->ack, TCP_RST, 0, 0);
			}

			return;
		}
	}

	// Check for RST
	if (flags & TCP_RST) {
		if (flags & TCP_ACK) {
			tcp_error(conn, TCP_CONN_RESET);
		}

		return;
	}

	// Check SYN
	if (flags & TCP_SYN) {
		// SYN is set.  ACK is either ok or there was no ACK.  No RST.

		conn->irs = hdr->seq;
		conn->rcv_nxt = hdr->seq + 1;

		if (flags & TCP_ACK) {
			conn->snd_una = hdr->ack;
			conn->snd_wnd = hdr->window_size;
			conn->snd_wl1 = hdr->seq;
			conn->snd_wl2 = hdr->ack;

			// TODO - Segments on the retransmission queue which are ack'd should be removed

			tcp_set_state(conn, TCP_ESTABLISHED);
			tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, 0, 0);


			// TODO - Data queued for transmission may be included with the ACK.

			// TODO - If there is data in the segment, continue processing at the URG phase.
		} else {
			tcp_set_state(conn, TCP_SYN_RECEIVED);

			// Resend ISS
			--conn->snd_nxt;
			tcp_send_packet(conn, conn->snd_nxt, TCP_SYN | TCP_ACK, 0, 0);
		}
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_rst(tcp_conn_t *conn, tcp_header_t *hdr)
{
	switch (conn->state) {
		case TCP_SYN_RECEIVED:
			// TODO - All segments on the retransmission queue should be removed

			// TODO - If initiated with a passive open, go to LISTEN state

			tcp_error(conn, TCP_CONN_REFUSED);
			break;

		case TCP_ESTABLISHED:
		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
		case TCP_CLOSE_WAIT:
			// TODO - All outstanding sends should receive "reset" responses

			tcp_error(conn, TCP_CONN_RESET);
			break;

		case TCP_CLOSING:
		case TCP_LAST_ACK:
		case TCP_TIME_WAIT:
			tcp_free(conn);
			break;
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_syn(tcp_conn_t *conn, tcp_header_t *hdr)
{
	// TODO - All outstanding sends should receive "reset" responses

	tcp_send_packet(conn, 0, TCP_RST | TCP_ACK, 0, 0);

	tcp_error(conn, TCP_CONN_RESET);
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_ack(tcp_conn_t *conn, tcp_header_t *hdr)
{
	switch (conn->state) {
		case TCP_SYN_RECEIVED:
			if (conn->snd_una <= hdr->ack && hdr->ack <= conn->snd_nxt) {
				conn->snd_wnd = hdr->window_size;
				conn->snd_wl1 = hdr->seq;
				conn->snd_wl2 = hdr->ack;
				tcp_set_state(conn, TCP_ESTABLISHED);
			} else {
				tcp_send_packet(conn, hdr->ack, TCP_RST, 0, 0);
			}
			break;

		case TCP_ESTABLISHED:
		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
		case TCP_CLOSE_WAIT:
		case TCP_CLOSING:
			// Handle expected acks
			if (SEQ_LE(conn->snd_una, hdr->ack) && SEQ_LE(hdr->ack, conn->snd_nxt)) {
				// Update acknowledged pointer
				conn->snd_una = hdr->ack;

				// Update send window
				if (SEQ_LT(conn->snd_wl1, hdr->seq) || (conn->snd_wl1 == hdr->seq && SEQ_LE(conn->snd_wl2, hdr->ack))) {
					conn->snd_wnd = hdr->window_size;
					conn->snd_wl1 = hdr->seq;
					conn->snd_wl2 = hdr->ack;
				}

				// TODO - remove segments on the retransmission queue which have been ack'd
				// TODO - acknowledge buffers which have sent to user
			}

			// Check for duplicate ack
			if (SEQ_LE(hdr->ack, conn->snd_una)) {
				// TODO - anything to do here?
			}

			// Check for ack of unsent data
			if (SEQ_GT(hdr->ack, conn->snd_nxt)) {
				tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, 0, 0);
				return;
			}

			// Check for ack of FIN
			if (SEQ_GE(hdr->ack, conn->snd_nxt)) {
				// TODO - is this the right way to detect that our FIN has been ACK'd?
				if (conn->state == TCP_FIN_WAIT_1) {
					tcp_set_state(conn, TCP_FIN_WAIT_2);
				}
				else if (conn->state == TCP_CLOSING) {
					tcp_set_state(conn, TCP_TIME_WAIT);
					conn->msl_wait = time(NULL) + 2 * TCP_MSL;
				}
			}
			break;

		case TCP_LAST_ACK:
			// Check for ack of FIN
			if (SEQ_GE(hdr->ack, conn->snd_nxt)) {
				// TODO - is this the right way to detect that our FIN has been ACK'd?
				tcp_free(conn);
			}
			break;

		case TCP_TIME_WAIT:
			// This case is handled in the FIN processing step.
			break;
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_insert(tcp_conn_t *conn, NetBuf *pkt)
{
	NetBuf *prev;
	NetBuf *cur;
	NetBuf *next;

	uint32_t data_len = pkt->end - pkt->start;
	uint32_t pktEnd = pkt->seq + data_len;

	// Find location to insert packet
	list_for_each(cur, conn->resequence, link)
	{
		if (SEQ_LE(pkt->seq, cur->seq)) {
			break;
		}
	}

	// Check if we already have some of this data in the previous packet.
	if (cur->link.prev != &conn->resequence) {
		prev = link_data(cur->link.prev, NetBuf, link);
		uint32_t prev_end = prev->seq + prev->end - prev->start;

		if (SEQ_GE(prev_end, pktEnd)) {
			// Complete overlap with queued packet - drop incoming packet
			NetReleaseBuf(pkt);
			return;
		} else if (SEQ_GT(prev_end, pkt->seq)) {
			// Trim previous packet by overlap with this packet
			prev->end -= prev_end - pkt->seq;
		}
	}

	// Remove all later packets if a FIN has been received
	if (pkt->flags & TCP_FIN) {
		while (&cur->link != &conn->resequence) {
			next = link_data(cur->link.next, NetBuf, link);
			link_remove(&cur->link);
			NetReleaseBuf(cur);
			cur = next;
		}
	}

	// Trim/remove later packets that overlap
	while (&cur->link != &conn->resequence) {
		uint32_t pktEnd = pkt->seq + data_len;
		uint32_t curEnd = cur->seq + cur->end - cur->start;

		if (SEQ_LT(pktEnd, cur->seq)) {
			// No overlap
			break;
		}

		if (SEQ_LT(pktEnd, curEnd)) {
			// Partial overlap - trim
			pkt->end -= pktEnd - cur->seq;
			break;
		}

		// Complete overlap - remove
		next = link_data(cur->link.next, NetBuf, link);
		link_remove(&cur->link);
		NetReleaseBuf(cur);
		cur = next;
	}

	// Add packet to the queue
	link_before(&cur->link, &pkt->link);
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_process(tcp_conn_t *conn)
{
	NetBuf *pkt;
	NetBuf *next;
	list_for_each_safe(pkt, next, conn->resequence, link)
	{
		if (conn->rcv_nxt != pkt->seq) {
			break;
		}

		uint32_t data_len = pkt->end - pkt->start;
		conn->rcv_nxt += data_len;

		if (conn->on_data) {
			conn->on_data(conn, pkt->start, data_len);
		}

		link_remove(&pkt->link);
		NetReleaseBuf(pkt);
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_data(tcp_conn_t *conn, NetBuf *pkt)
{
	switch (conn->state)
	{
		case TCP_SYN_RECEIVED:
			// TODO - can this happen? ACK processing would transition to ESTABLISHED state.
			break;

		case TCP_ESTABLISHED:
		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
			// Increase ref count on packet
			++pkt->ref_count;

			// Insert packet on to input queue sorted by sequence
			tcp_recv_insert(conn, pkt);

			// Process packets that are now in order
			tcp_recv_process(conn);
	
			// Acknowledge receipt of data
			tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, 0, 0);
			break;

		default:
			// FIN has been received from the remote side - ignore the segment data.
			break;
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_fin(tcp_conn_t *conn, tcp_header_t *hdr)
{
	// TODO - signal the user "connection closing" and return any pending receives

	conn->rcv_nxt = hdr->seq + 1;
	tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, 0, 0);

	switch (conn->state) {
		case TCP_SYN_RECEIVED:
		case TCP_ESTABLISHED:
			tcp_set_state(conn, TCP_CLOSE_WAIT);
			break;

		case TCP_FIN_WAIT_1:
			if (SEQ_GE(hdr->ack, conn->snd_nxt)) {
				// TODO - is this the right way to detect that our FIN has been ACK'd?
				// TODO - turn off the other timers
				tcp_set_state(conn, TCP_TIME_WAIT);
				conn->msl_wait = time(NULL) + 2 * TCP_MSL;
			} else {
				tcp_set_state(conn, TCP_CLOSING);
			}
			break;

		case TCP_FIN_WAIT_2:
			// TODO - turn off the other timers
			tcp_set_state(conn, TCP_TIME_WAIT);
			conn->msl_wait = time(NULL) + 2 * TCP_MSL;
			break;

		case TCP_CLOSE_WAIT:
		case TCP_CLOSING:
		case TCP_LAST_ACK:
			break;

		case TCP_TIME_WAIT:
			conn->msl_wait = time(NULL) + 2 * TCP_MSL;
			break;
	}
}

// ------------------------------------------------------------------------------------------------
static void tcp_recv_general(tcp_conn_t *conn, tcp_header_t *hdr, NetBuf *pkt)
{
	// Process segments not in the CLOSED, LISTEN, or SYN-SENT states.

	uint32_t flags = hdr->flags;
	uint32_t data_len = pkt->end - pkt->start;

	// Check that sequence and segment data is acceptable
	if (!(SEQ_LE(conn->rcv_nxt, hdr->seq) && SEQ_LE(hdr->seq + data_len, conn->rcv_nxt + conn->rcv_wnd))) {
		// Unacceptable segment
		if (~flags & TCP_RST) {
			tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, 0, 0);
		}
		return;
	}

	// TODO - trim segment data?

	// Check RST bit
	if (flags & TCP_RST) {
		tcp_recv_rst(conn, hdr);
		return;
	}

	// Check SYN bit
	if (flags & TCP_SYN) {
		tcp_recv_syn(conn, hdr);
	}

	// Check ACK
	if (~flags & TCP_ACK) {
		return;
	}

	tcp_recv_ack(conn, hdr);

	// TODO - check URG

	// Process segment data
	if (data_len) {
		tcp_recv_data(conn, pkt);
	}

	// Check FIN - TODO, needs to handle out of sequence
	if (flags & TCP_FIN) {
		tcp_recv_fin(conn, hdr);
	}
}

// ------------------------------------------------------------------------------------------------
void tcp_recv(NetIntf *intf, const ip_packet_t *ip_header, NetBuf *pkt)
{
	// Validate packet header
	if (pkt->start + sizeof(tcp_header_t) > pkt->end) {
		return;
	}

	// Assemble Pseudo Header
	uint32_t srcAddr = *((uint32_t)(&ip_header->src_ip));
	uint32_t dstAddr = *((uint32_t)(&ip_header->dst_ip));
	uint8_t protocol = ip_header->protocol;

	tcp_checksummed_t *phdr = (tcp_checksummed_t *)(pkt->start - sizeof(tcp_checksummed_t));
	phdr->src = srcAddr;
	phdr->dst = dstAddr;
	phdr->reserved = 0;
	phdr->protocol = protocol;
	phdr->len = ntohs(pkt->end - pkt->start);

	tcp_debug(pkt);

	// TODO: Validate checksum

	// Process packet
	tcp_header_t *hdr = (tcp_header_t *)pkt->start;
	hdr->src_port = ntohs(hdr->src_port);
	hdr->dst_port = ntohs(hdr->dst_port);
	hdr->seq = ntohl(hdr->seq);
	hdr->ack = ntohl(hdr->ack);
	hdr->window_size = ntohs(hdr->window_size);
	hdr->checksum = ntohs(hdr->checksum);
	hdr->urgent = ntohs(hdr->urgent);
	phdr->len = ntohs(phdr->len);

	// Find connection associated with packet
	tcp_conn_t *conn = tcp_find(phdr->src, hdr->src_port, phdr->dst, hdr->dst_port);
	if (!conn || conn->state == TCP_CLOSED) {
		tcp_recv_closed(phdr, hdr);
		return;
	}

	// Process packet by state
	if (conn->state == TCP_LISTEN) {
	}
	else if (conn->state == TCP_SYN_SENT) {
		tcp_recv_syn_sent(conn, hdr);
	} else {
		// Update packet to point to data, and store parts of
		// header needed for out of order handling.
		uint32_t hdrLen = hdr->off >> 2;
		pkt->start += hdrLen;
		pkt->seq = hdr->seq;
		pkt->flags = hdr->flags;

		tcp_recv_general(conn, hdr, pkt);
	}
}

// ------------------------------------------------------------------------------------------------
void tcp_poll()
{
	tcp_conn_t *conn;
	tcp_conn_t *next;
	list_for_each_safe(conn, next, active_connections, link)
	{
		if (conn->state == TCP_TIME_WAIT && SEQ_GE(time(NULL), conn->msl_wait)) {
			tcp_free(conn);
		}
	}
}

// ------------------------------------------------------------------------------------------------
tcp_conn_t *tcp_create()
{
	tcp_conn_t *conn = tcp_alloc();
	memset(conn, 0, sizeof(tcp_conn_t));
	conn->resequence.next = &conn->resequence;
	conn->resequence.prev = &conn->resequence;

	return conn;
}

// ------------------------------------------------------------------------------------------------
bool tcp_connect(tcp_conn_t *conn, const uint32_t addr, uint16_t port)
{
	// Initialize connection
	conn->local_addr = intf->ipAddr;
	conn->remote_addr = addr;
	conn->local_port = 1025; // XXX FIXME this is temporary
	conn->remote_port = port;

	uint32_t isn = base_isn++;

	conn->snd_una = isn;
	conn->snd_nxt = isn;
	conn->snd_wnd = TCP_WINDOW_SIZE;
	conn->snd_up = 0;
	conn->snd_wl1 = 0;
	conn->snd_wl2 = 0;
	conn->iss = isn;

	conn->rcv_nxt = 0;
	conn->rcv_wnd = TCP_WINDOW_SIZE;
	conn->rcv_up = 0;
	conn->irs = 0;

	// link to active connections
	link_before(&active_connections, &conn->link);

	// Issue SYN segment
	tcp_send_packet(conn, conn->snd_nxt, TCP_SYN, 0, 0);
	tcp_set_state(conn, TCP_SYN_SENT);

	return true;
}

// ------------------------------------------------------------------------------------------------
void tcp_close(tcp_conn_t *conn)
{
	switch (conn->state) {
		case TCP_CLOSED:
			tcp_free(conn);
			break;

		case TCP_LISTEN:
			tcp_free(conn);
			break;

		case TCP_SYN_SENT:
			// TODO - cancel queued sends
			tcp_free(conn);
			break;

		case TCP_SYN_RECEIVED:
			// TODO - if sends have been issued or queued, wait for ESTABLISHED
			// before entering FIN-WAIT-1
			tcp_send_packet(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			break;

		case TCP_ESTABLISHED:
			// TODO - queue FIN after sends
			tcp_send_packet(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
			tcp_set_state(conn, TCP_FIN_WAIT_1);
			break;

		case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
		case TCP_CLOSING:
		case TCP_LAST_ACK:
		case TCP_TIME_WAIT:
			if (conn->on_error) {
				conn->on_error(conn, TCP_CONN_CLOSING);
			}
			break;

		case TCP_CLOSE_WAIT:
			// TODO - queue FIN and state transition after sends
			tcp_send_packet(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
			tcp_set_state(conn, TCP_LAST_ACK);
			break;
	}
}

// ------------------------------------------------------------------------------------------------
void tcp_send(tcp_conn_t *conn, const void *data, uint32_t count)
{
	tcp_send_packet(conn, conn->snd_nxt, TCP_ACK, data, count);
}

