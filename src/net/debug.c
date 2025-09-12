#include <kernel.h>

void tcp_dump_segment(bool in, tcp_conn_t* conn, const ip_packet_t* encap_packet, const tcp_segment_t* segment, const tcp_options_t* options, size_t len, uint16_t our_checksum)
{
#ifdef TCP_TRACE
	const char *states[] = { "LISTEN","SYN-SENT","SYN-RECEIVED","ESTABLISHED","FIN-WAIT-1","FIN-WAIT-2","CLOSE-WAIT","CLOSING","LAST-ACK","TIME-WAIT" };
	char source_ip[15] = { 0 }, dest_ip[15] = { 0 };
	get_ip_str(source_ip, encap_packet->src_ip);
	get_ip_str(dest_ip, encap_packet->dst_ip);
	dprintf(
		"TCP %s: %s %s:%u->%s:%u len=%lu seq=%u ack=%u off=%u flags[%c%c%c%c%c%c%c%c] win=%u, sum=%04x/%04x, urg=%u",
		in ? "IN" : "OUT",
		conn ? states[conn->state] : "<invalid>",
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
		dprintf(" [opt.mss=%u]", options->mss);
	}
	dprintf("\n");
#endif
}
