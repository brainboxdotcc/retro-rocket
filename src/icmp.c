#include <kernel.h>

uint16_t icmp_calculate_checksum(void* packet, size_t len)
{
	uint32_t sum = 0;
	uint8_t* bytes = (uint8_t*)packet;

	for (size_t i = 0; i + 1 < len; i += 2) {
		uint16_t word = (bytes[i] << 8) | bytes[i + 1];
		sum += word;
	}

	if (len & 1) {
		uint16_t last = bytes[len - 1] << 8;
		sum += last;
	}

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}

void icmp_send(uint32_t destination, void* icmp, uint16_t size)
{
	dump_hex(icmp, size);
	ip_send_packet((uint8_t*)&destination, icmp, size, PROTOCOL_ICMP);
}

void icmp_send_echo(uint32_t destination, uint16_t id, uint16_t seq)
{
	icmp_echo_packet_t echo = {
		.type = ICMP_ECHO,
		.code = 0,
		.checksum = 0,
		.id = htons(id),
		.seq = htons(seq),
	};
	echo.checksum = htons(icmp_calculate_checksum(&echo, sizeof(icmp_echo_packet_t)));
	icmp_send(destination, &echo, sizeof(icmp_echo_packet_t));
}

void icmp_send_parameter_problem(ip_packet_t* original_ip, uint8_t pointer_offset)
{
	uint8_t buf[sizeof(icmp_parameter_problem_packet_t)];
	icmp_parameter_problem_packet_t* p = (icmp_parameter_problem_packet_t*)buf;

	memset(p, 0, sizeof(*p));
	p->type = ICMP_PARAMETER_PROBLEM;
	p->code = 0;
	p->problem_ptr = pointer_offset;

	// Include original IP header
	size_t copy_len = sizeof(ip_packet_t);
	if (original_ip->length < 28) return;
	memcpy(&p->original_datagram, original_ip, copy_len);

	p->checksum = htons(icmp_calculate_checksum(p, sizeof(*p)));

	icmp_send(original_ip->src_ip, p, sizeof(*p));
}

void icmp_send_redirect(ip_packet_t* original_ip, uint8_t code, uint32_t new_gateway_ip)
{
	uint8_t buf[sizeof(icmp_redirect_packet_t)];
	icmp_redirect_packet_t* p = (icmp_redirect_packet_t*)buf;

	memset(p, 0, sizeof(*p));
	p->type = ICMP_REDIRECT;
	p->code = code;
	p->gateway = new_gateway_ip;

	// Embed original packet header
	size_t copy_len = sizeof(ip_packet_t);
	if (original_ip->length < 28) return;
	memcpy(&p->original_datagram, original_ip, copy_len);

	p->checksum = htons(icmp_calculate_checksum(p, sizeof(*p)));

	icmp_send(original_ip->src_ip, p, sizeof(*p));
}

void icmp_send_fragmentation_needed(ip_packet_t* original_ip, uint16_t mtu)
{
	uint8_t buf[sizeof(icmp_packet_t)];
	icmp_packet_t* p = (icmp_packet_t*)buf;

	memset(p, 0, sizeof(*p));
	p->type = ICMP_DESTINATION_UNREACHABLE;
	p->code = ICMP_FRAGMENTATION_NEEDED;

	// RFC 1191: put MTU in unused field (host order)
	p->unused = htonl(mtu);

	size_t copy_len = sizeof(ip_packet_t);
	if (original_ip->length < 28) return;
	memcpy(&p->original_datagram, original_ip, copy_len);

	p->checksum = htons(icmp_calculate_checksum(p, sizeof(*p)));

	icmp_send(original_ip->src_ip, p, sizeof(*p));
}

void icmp_send_echo_reply(ip_packet_t* encap_packet, icmp_echo_packet_t* req, size_t len)
{
	if (len < sizeof(icmp_echo_packet_t)) {
		kprintf("Echo packet too small\n");
		return;
	}
	// Reuse original request buffer for reply
	uint8_t reply_buf[576]; // Max safe ICMP size
	if (len > sizeof(reply_buf)) len = sizeof(reply_buf);

	memcpy(reply_buf, req, len);
	icmp_echo_packet_t* reply = (icmp_echo_packet_t*)reply_buf;

	reply->type = ICMP_ECHO_REPLY;
	reply->code = 0;
	reply->id = req->id;
	reply->seq = req->seq;
	reply->checksum = 0;
	uint16_t sum = icmp_calculate_checksum(reply_buf, len);
	reply->checksum = htons(sum);

	char src_ip[20];
	get_ip_str(src_ip, encap_packet->src_ip);
	kprintf("ICMP TX: to IP %s len=%d id=%04x seq=%04x sum=%04x\n", src_ip, len, ntohs(reply->id), ntohs(reply->seq), sum);
	icmp_send(encap_packet->src_ip, reply_buf, (uint16_t)len);
}

void icmp_handle_echo_packet(ip_packet_t* encap_packet, icmp_echo_packet_t* packet, size_t len)
{
	icmp_send_echo_reply(encap_packet, packet, len);
}

void icmp_handle_echo_reply_packet(ip_packet_t* encap_packet, icmp_echo_packet_t* packet, size_t len)
{
	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ECHO echo ECHO! Got ICMP reply FROM %s, seq=%d id=%d (len=%zu)\n", ip, ntohs(packet->seq), ntohs(packet->id), len);
}

void icmp_handle_destination_unreachable_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_packet_t)) return;

	const char* reason = "Unknown";
	switch (packet->code) {
		case ICMP_NET_UNREACHABLE: reason = "Network unreachable"; break;
		case ICMP_HOST_UNREACHABLE: reason = "Host unreachable"; break;
		case ICMP_PROTOCOL_UNREACHABLE: reason = "Protocol unreachable"; break;
		case ICMP_PORT_UNREACHABLE: reason = "Port unreachable"; break;
		case ICMP_FRAGMENTATION_NEEDED: reason = "Fragmentation needed"; break;
		case ICMP_SOURCE_ROUTE_FAILED: reason = "Source route failed"; break;
	}

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Destination Unreachable from %s: %s\n", ip, reason);
}

void icmp_handle_source_quench_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Source Quench from %s — deprecated, ignoring\n", ip);
}

void icmp_handle_redirect_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_redirect_packet_t)) return;

	icmp_redirect_packet_t* redirect = (icmp_redirect_packet_t*)packet;

	const char* target = "Unknown";
	switch (redirect->code) {
		case ICMP_REDIRECT_NETWORK: target = "network"; break;
		case ICMP_REDIRECT_HOST: target = "host"; break;
		case ICMP_REDIRECT_TOS_NETWORK: target = "TOS/network"; break;
		case ICMP_REDIRECT_TOS_HOST: target = "TOS/host"; break;
	}

	char src[14], new_gateway[14];
	get_ip_str(src, encap_packet->src_ip);
	get_ip_str(new_gateway, redirect->gateway);

	dprintf("ICMP Redirect from %s: Redirect %s traffic to gateway %s\n", src, target, new_gateway);
}

void icmp_handle_time_exceeded_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_packet_t)) return;

	const char* reason = packet->code == ICMP_FRAGMENT_REASSEMBLY_TIME_EXCEEDED ?
			     "Fragment reassembly time exceeded" : "TTL expired";

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Time Exceeded from %s: %s\n", ip, reason);
}

void icmp_handle_parameter_problem_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_parameter_problem_packet_t)) return;

	icmp_parameter_problem_packet_t* prob = (icmp_parameter_problem_packet_t*)packet;

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Parameter Problem from %s: byte %u is invalid in header\n", ip, prob->problem_ptr);
}

void icmp_handle_timestamp_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_timestamp_packet_t)) return;

	icmp_timestamp_packet_t* ts = (icmp_timestamp_packet_t*)packet;

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Timestamp Request from %s: id=%u, seq=%u\n", ip, ntohs(ts->id), ntohs(ts->seq));

	// TODO: implement a reply if time is tracked
}

void icmp_handle_timestamp_reply_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_timestamp_packet_t)) return;

	icmp_timestamp_packet_t* ts = (icmp_timestamp_packet_t*)packet;

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Timestamp Reply from %s: id=%u, seq=%u, time=%u\n",
		ip, ntohs(ts->id), ntohs(ts->seq), ntohl(ts->receive_timestamp));
}

void icmp_handle_information_request_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_information_t)) return;

	icmp_information_t* info = (icmp_information_t*)packet;

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Information Request from %s: id=%u, seq=%u\n", ip, ntohs(info->id), ntohs(info->seq));
}

void icmp_handle_information_reply_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	if (len < sizeof(icmp_information_t)) return;

	icmp_information_t* info = (icmp_information_t*)packet;

	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	dprintf("ICMP Information Reply from %s: id=%u, seq=%u\n", ip, ntohs(info->id), ntohs(info->seq));
}

void icmp_send_destination_unreachable(ip_packet_t* original_ip, uint8_t code)
{
	uint8_t buf[sizeof(icmp_packet_t)];

	icmp_packet_t* p = (icmp_packet_t*)buf;
	memset(p, 0, sizeof(icmp_packet_t));

	p->type = ICMP_DESTINATION_UNREACHABLE;
	p->code = code;
	p->checksum = 0;

	// Include the original IP header and first 8 bytes of its payload
	size_t copy_len = sizeof(ip_packet_t);
	if (original_ip->length < 28) return; // Not enough to include
	memcpy(&p->original_datagram, original_ip, copy_len);

	p->checksum = htons(icmp_calculate_checksum(p, sizeof(icmp_packet_t)));

	icmp_send(original_ip->src_ip, p, sizeof(icmp_packet_t));
}

void icmp_send_time_exceeded(ip_packet_t* original_ip, uint8_t code)
{
	uint8_t buf[sizeof(icmp_packet_t)];

	icmp_packet_t* p = (icmp_packet_t*)buf;
	memset(p, 0, sizeof(icmp_packet_t));

	p->type = ICMP_TIME_EXCEEDED;
	p->code = code;
	p->checksum = 0;

	// Include the original IP header and first 8 bytes of its payload
	size_t copy_len = sizeof(ip_packet_t);
	if (original_ip->length < 28) return;
	memcpy(&p->original_datagram, original_ip, copy_len);

	p->checksum = htons(icmp_calculate_checksum(p, sizeof(icmp_packet_t)));

	icmp_send(original_ip->src_ip, p, sizeof(icmp_packet_t));
}

void icmp_handle_packet(ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	uint16_t received_sum = packet->checksum;      // Save the existing checksum
	packet->checksum = 0;                          // Zero it for calculation
	uint16_t calc_sum = icmp_calculate_checksum(packet, len);
	packet->checksum = received_sum;               // Restore it

	kprintf("ICMP RX: type=%d code=%d received_sum=%04x calculated_sum=%04x\n", packet->type, packet->code, received_sum, calc_sum);

	switch (packet->type) {
		case ICMP_ECHO_REPLY:
			icmp_handle_echo_reply_packet(encap_packet, (icmp_echo_packet_t*)packet, len);
			break;
		case ICMP_DESTINATION_UNREACHABLE:
			icmp_handle_destination_unreachable_packet(encap_packet, packet, len);
			break;
		case ICMP_SOURCE_QUENCH:
			icmp_handle_source_quench_packet(encap_packet, packet, len);
			break;
		case ICMP_REDIRECT:
			icmp_handle_redirect_packet(encap_packet, packet, len);
			break;
		case ICMP_ECHO:
			icmp_handle_echo_packet(encap_packet, (icmp_echo_packet_t*)packet, len);
			break;
		case ICMP_TIME_EXCEEDED:
			icmp_handle_time_exceeded_packet(encap_packet, packet, len);
			break;
		case ICMP_PARAMETER_PROBLEM:
			icmp_handle_parameter_problem_packet(encap_packet, packet, len);
			break;
		case ICMP_TIMESTAMP:
			icmp_handle_timestamp_packet(encap_packet, packet, len);
			break;
		case ICMP_TIMESTAMP_REPLY:
			icmp_handle_timestamp_reply_packet(encap_packet, packet, len);
			break;
		case ICMP_INFORMATION_REQUEST:
			icmp_handle_information_request_packet(encap_packet, packet, len);
			break;
		case ICMP_INFORMATION_REPLY:
			icmp_handle_information_reply_packet(encap_packet, packet, len);
			break;
		default:
			dprintf("*** WARN *** Unknown ICMP type %d on inbound packet\n", packet->type);
			break;
	}
}
