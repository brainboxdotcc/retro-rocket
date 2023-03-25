#include <kernel.h>

void tcp_byte_order(tcp_segment_t* segment);

uint16_t tcp_calculate_checksum(ip_packet_t* packet, tcp_segment_t* segment, size_t len)
{
	int array_size = len + sizeof(tcp_ip_pseudo_header_t);
	tcp_ip_pseudo_header_t* pseudo = kmalloc(array_size * 2);
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

void tcp_dump_segment(ip_packet_t* encap_packet, tcp_segment_t* segment, tcp_options_t* options, size_t len, uint16_t our_checksum)
{
	kprintf(
		"TCP: len=%ld src_port=%d dst_port=%d seq=%d ack=%d off=%d\n\
flags[fin=%d,syn=%d,rst=%d,psh=%d,ack=%d,urg=%d,ece=%d,cwr=%d]\n\
window_size=%d,checksum=0x%04x (%s), urgent=%d options[mss=%d (%04x)]\n\n",
		len,
		segment->src_port,
		segment->dst_port,
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
		our_checksum == segment->checksum ? "ok" : "bad",
		segment->urgent,
		options->mss,
		options->mss
	);
}

void tcp_byte_order(tcp_segment_t* segment)
{
	segment->src_port = ntohs(segment->src_port);
	segment->dst_port = ntohs(segment->dst_port);
	segment->seq = ntohl(segment->seq);
	segment->ack = ntohl(segment->ack);
	segment->window_size = ntohs(segment->window_size);
	segment->checksum = ntohs(segment->checksum);
	segment->urgent = ntohs(segment->urgent);
}

void tcp_parse_options(tcp_segment_t* segment, tcp_options_t* options)
{
	uint8_t* opt_ptr = segment->options;
	options->mss = 0;

	while (opt_ptr < (uint8_t*)segment + (segment->flags.off * 4) && *opt_ptr != TCP_OPT_END) {
		uint8_t opt_size = *(opt_ptr + 1);
		switch (*opt_ptr) {
			case TCP_OPT_MSS:
				options->mss = ntohs(*((uint16_t*)(opt_ptr + 2)));
			break;
			default:
			break;
		}
		opt_ptr += (opt_size ? opt_size : 1);
	}
	return;
}

void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len)
{
	tcp_options_t options;
	uint16_t our_checksum = tcp_calculate_checksum(encap_packet, segment, len);
	tcp_byte_order(segment);
	tcp_parse_options(segment, &options);
	tcp_dump_segment(encap_packet, segment, &options, len, our_checksum);
	if (our_checksum == segment->checksum) {

	}
}