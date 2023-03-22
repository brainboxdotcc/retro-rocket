#include <kernel.h>

uint16_t icmp_calculate_checksum(void* packet, size_t len)
{
	int array_size = len / 2;
	uint16_t * array = (uint16_t*)packet; // XXX: Alignment!
	uint32_t sum = 0;
	for(int i = 0; i < array_size; i++) {
		sum += htons(array[i]);
	}
	uint32_t carry = sum >> 16;
	sum = sum & 0x0000ffff;
	sum = sum + carry;
	uint16_t ret = ~sum;
	return htons(ret);
}

void icmp_send(uint32_t destination, void* icmp, uint16_t size)
{
	ip_send_packet((uint8_t*)&destination, icmp, size, PROTOCOL_ICMP);
}

void icmp_send_echo(uint32_t destination, uint16_t id, uint16_t seq)
{
	icmp_echo_packet_t echo = { .type = ICMP_ECHO, .code = 0, .checksum = 0, .id = htons(id), .seq = htons(seq) };
	echo.checksum = icmp_calculate_checksum(&echo, (uint16_t)sizeof(icmp_echo_packet_t));
	icmp_send(destination, &echo, (uint16_t)sizeof(icmp_echo_packet_t));
}

void icmp_handle_echo_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_echo_packet_t* packet, size_t len)
{
	/* TODO: Send reply here */
}

void icmp_handle_echo_reply_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_echo_packet_t* packet, size_t len)
{
	char ip[14];
	get_ip_str(ip, encap_packet->src_ip);
	kprintf("ECHO echo ECHO! Got ICMP reply FROM %s, seq=%d id=%d\n", ip, ntohs(packet->seq), ntohs(packet->id));
	//icmp_send_echo(0x0202000a);

}

void icmp_handle_destination_unreachable_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
}

void icmp_handle_source_quench_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
}

void icmp_handle_redirect_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_redirect_packet_t* packet, size_t len)
{
}

void icmp_handle_time_exceeded_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
}

void icmp_handle_parameter_problem_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_parameter_problem_packet_t* packet, size_t len)
{
}

void icmp_handle_timestamp_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_timestamp_packet_t* packet, size_t len)
{
}

void icmp_handle_timestamp_reply_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_timestamp_packet_t* packet, size_t len)
{
}

void icmp_handle_information_request_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_information_t* packet, size_t len)
{
}

void icmp_handle_information_reply_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_information_t* packet, size_t len)
{
}

void icmp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len)
{
	void* p = packet;
	switch (packet->type) {
		case ICMP_ECHO_REPLY:
			icmp_handle_echo_reply_packet(encap_packet, p, len);
			break;
		case ICMP_DESTINATION_UNREACHABLE:
			icmp_handle_destination_unreachable_packet(encap_packet, p, len);
			break;
		case ICMP_SOURCE_QUENCH:
			icmp_handle_source_quench_packet(encap_packet, p, len);
			break;
		case ICMP_REDIRECT:
			icmp_handle_redirect_packet(encap_packet, p, len);
			break;
		case ICMP_ECHO:
			icmp_handle_echo_packet(encap_packet, p, len);
			break;
		case ICMP_TIME_EXCEEDED:
			icmp_handle_time_exceeded_packet(encap_packet, p, len);
			break;
		case ICMP_PARAMETER_PROBLEM:
			icmp_handle_parameter_problem_packet(encap_packet, p, len);
			break;
		case ICMP_TIMESTAMP:
			icmp_handle_timestamp_packet(encap_packet, p, len);
			break;
		case ICMP_TIMESTAMP_REPLY:
			icmp_handle_timestamp_reply_packet(encap_packet, p, len);
			break;
		case ICMP_INFORMATION_REQUEST:
			icmp_handle_information_request_packet(encap_packet, p, len);
			break;
		case ICMP_INFORMATION_REPLY:
			icmp_handle_information_reply_packet(encap_packet, p, len);
			break;
		default:
			kprintf("*** WARN *** Unknown icmp type %d on inbound packet\n", packet->type);
			break;
	}
}