#include <kernel.h>

static uint32_t id = 1;

static void buffer_to_packet(dns_header_t *header, const unsigned char *input, const int length)
{
	header->id[0] = input[0];
	header->id[1] = input[1];
	header->flags1 = input[2];
	header->flags2 = input[3];
	header->qdcount = (input[4] << 8) + input[5];
	header->ancount = (input[6] << 8) + input[7];
	header->nscount = (input[8] << 8) + input[9];
	header->arcount = (input[10] << 8) + input[11];
	memcpy(header->payload,&input[12],length);
}

static void packet_to_buffer(unsigned char *output, const dns_header_t *header, const int length)
{
	output[0] = header->id[0];
	output[1] = header->id[1];
	output[2] = header->flags1;
	output[3] = header->flags2;
	output[4] = header->qdcount >> 8;
	output[5] = header->qdcount & 0xFF;
	output[6] = header->ancount >> 8;
	output[7] = header->ancount & 0xFF;
	output[8] = header->nscount >> 8;
	output[9] = header->nscount & 0xFF;
	output[10] = header->arcount >> 8;
	output[11] = header->arcount & 0xFF;
	memcpy(&output[12],header->payload,length);
}

static int dns_send_request(uint32_t resolver_ip, dns_request_t* request, const dns_header_t *header, const int length, uint8_t query_type)
{
	unsigned char payload[sizeof(dns_header_t)];

	request->rr_class = 1;
	request->type = query_type;

	packet_to_buffer(payload, header, length);

	udp_send_packet((uint8_t*)&resolver_ip, DNS_SRC_PORT, DNS_DST_PORT, payload, length + 12);

	return 1;
}

static int dns_make_payload(const char * const name, const uint8_t rr, const unsigned short rr_class, unsigned char * const payload)
{
	short payloadpos = 0;
	const char* tempchr, *tempchr2 = name;
	unsigned short length;

	/* split name up into labels, create query */
	while ((tempchr = strchr(tempchr2,'.')) != NULL)
	{
		length = tempchr - tempchr2;
		if (payloadpos + length + 1 > 507)
			return -1;
		payload[payloadpos++] = length;
		memcpy(&payload[payloadpos],tempchr2,length);
		payloadpos += length;
		tempchr2 = &tempchr[1];
	}
	length = strlen(tempchr2);
	if (length)
	{
		if (payloadpos + length + 2 > 507)
			return -1;
		payload[payloadpos++] = length;
		memcpy(&payload[payloadpos],tempchr2,length);
		payloadpos += length;
		payload[payloadpos++] = 0;
	}
	if (payloadpos > 508)
		return -1;
	length = htons(rr);
	memcpy(&payload[payloadpos],&length,2);
	length = htons(rr_class);
	memcpy(&payload[payloadpos + 2],&length,2);
	return payloadpos + 4;
}

int dns_lookup_host(uint32_t resolver_ip, const char* hostname)
{
	dns_request_t request;
	dns_header_t h;
	int length;

	if ((length = dns_make_payload(hostname, DNS_QUERY_A, 1, (unsigned char*)&h.payload)) == -1)
		return -1;

	h.id[0] = request.id[0] = id >> 8;
	h.id[1] = request.id[1] = id++ & 0xFF;
	h.flags1 = FLAGS_MASK_RD;
	h.flags2 = 0;
	h.qdcount = 1;
	h.ancount = 0;
	h.nscount = 0;
	h.arcount = 0;

	dns_send_request(resolver_ip, &request, &h, length, DNS_QUERY_A);

	return length;
}

void dns_handle_packet([[maybe_unused]] uint16_t dst_port, void* data, uint32_t length) {
	dns_header_t* packet = (dns_header_t*)data;
	kprintf("DNS packet id %d\n", (uint16_t)&packet->id);
	icmp_send_echo(0x0302000a, 69, 1);
}

void init_dns()
{
	udp_register_daemon(DNS_SRC_PORT, &dns_handle_packet);
}