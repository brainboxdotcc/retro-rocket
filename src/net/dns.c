#include <kernel.h>

static uint16_t id = 1;
uint16_t dns_query_port = 0;
static struct hashmap* dns_replies = NULL;
static struct hashmap* dns_cache = NULL;

void dns_result_ready(dns_header_t* header, dns_request_t* request, unsigned length, char** error, char* res, uint8_t* outlength);
uint8_t dns_collect_request(uint16_t id, char* result, size_t max);

/**
 * @brief Comparison function for hash table of DNS requests
 * 
 * @param a first object to compare
 * @param b second object to compare
 * @param udata user data
 * @return int 0 for equal, -1 for less than, 1 for greater than; like strcmp()
 */
static int dns_request_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
    const dns_request_t* fa = a;
    const dns_request_t* fb = b;
    return fa->id == fb->id ? 0 : (fa->id < fb->id ? -1 : 1);
}

static int dns_cache_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
	const dns_cache_entry_t* fa = a;
	const dns_cache_entry_t* fb = b;
	return strcmp(fa->host, fb->host);
}

/**
 * @brief Hash two DNS requests by ID
 * 
 * @param item item to hash
 * @param seed0 first seed from hashmap
 * @param seed1 second seed from hashmap
 * @return uint64_t hash bucket value
 */
static uint64_t dns_request_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const dns_request_t* header = item;
    return (uint64_t)header->id * seed0 * seed1;
}

static uint64_t dns_cache_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const dns_cache_entry_t* header = item;
	uint64_t seed = 0, pos = 0;
	for (const char* x = header->host; *x; ++x) {
		seed += *x * pos++;
	}
	return (uint64_t)seed * seed0 * seed1;
}

/**
 * @brief Convert a packet to a buffer
 * 
 * @param output output buffer
 * @param header header to convert
 * @param length length of converted packet
 */
static void packet_to_buffer(unsigned char *output, const dns_header_t *header, const int length)
{
	*((uint16_t*)(&output[0])) = htons(header->id);
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

/**
 * @brief Send a DNS request, and increment the next request ID
 * 
 * @param name A string representation of the address or hostname being resolved.
 * This is purely for reference purposes.
 * @param resolver_ip IP address of the resolver
 * @param request Request information, contains details of the request and any callbacks
 * @param header Header for the DNS packet
 * @param length length of request packet
 * @param query_type Query type to send, e.g. A, AAAA, PTR, CNAME
 * @return int zero if request could not be sent, e.g. due to network stack not up
 */
static int dns_send_request(const char * const name, uint32_t resolver_ip, dns_request_t* request, dns_header_t *header, const int length, uint8_t query_type)
{
	unsigned char payload[length + 12];

	if (dns_query_port == 0 || resolver_ip == 0) {
		return 0;
	}

	dprintf("dns_send_request(%s, %08x, %d)\n", name, resolver_ip, query_type);

	request->rr_class = 1;
	request->orig = (unsigned char*)strdup(name);
	request->type = query_type;
	request->id = id++;
	header->id = request->id;
	*(request->result) = 0;
	request->result_length = 0;

	/* ID 0 has special significance for errors */
	if (id == 0) {
		id = 1;
	}

	packet_to_buffer(payload, header, length);
	hashmap_set(dns_replies, request);

	udp_send_packet((uint8_t*)&resolver_ip, dns_query_port, DNS_DST_PORT, payload, length + 12);

	return 1;
}

/**
 * @brief Build a binary payload for a request for a specific type of DNS entry
 *
 * @param name name to resolve. Even reverse DNS uses these label-separated names
 * @param rr Resource record - identifies the type of record, e.g. A, AAAA, PTR, CNAME
 * @param rr_class Resource record class
 * @param payload Packet payload buffer to fill with data
 * @return int length of filled buffer
 */
static int dns_make_payload(const char * const name, const uint8_t rr, const unsigned short rr_class, unsigned char * const payload)
{
	short payloadpos = 0;
	const char* tempchr, *tempchr2 = name;
	unsigned short length;

	/* split name up into labels, create query */
	while ((tempchr = strchr(tempchr2, '.')) != NULL) {
		length = tempchr - tempchr2;
		if (payloadpos + length + 1 > 507)
			return -1;
		payload[payloadpos++] = length;
		memcpy(&payload[payloadpos],tempchr2,length);
		payloadpos += length;
		tempchr2 = &tempchr[1];
	}
	length = strlen(tempchr2);
	if (length) {
		if (payloadpos + length + 2 > 507) {
			return -1;
		}
		payload[payloadpos++] = length;
		memcpy(&payload[payloadpos], tempchr2, length);
		payloadpos += length;
		payload[payloadpos++] = 0;
	}
	if (payloadpos > 508) {
		return -1;
	}
	length = htons(rr);
	memcpy(&payload[payloadpos],&length,2);
	length = htons(rr_class);
	memcpy(&payload[payloadpos + 2],&length,2);
	return payloadpos + 4;
}

/**
 * @brief Handle inbound packet from the IP stack
 * 
 * @param dst_port destination UDP port
 * @param data raw packet data
 * @param length length of packet
 */
void dns_handle_packet(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t length, void* opaque) {
	dns_header_t* packet = (dns_header_t*)data;
	uint16_t inbound_id = ntohs(packet->id);
	dns_request_t findrequest = { .id = inbound_id };
	dns_request_t* request = (dns_request_t*)hashmap_get(dns_replies, &findrequest);
	dprintf("dns inbound packet of size %d\n", length);
	if (request) {
		add_random_entropy(*(uint64_t*)packet);
		if (request->result_length == 0) {
			dprintf("No processed result yet\n");
			char* error = NULL;
			dns_result_ready(packet, request, length, &error, request->result, &request->result_length);
			dprintf("DNS result ready done\n");
			if (request->callback_a && request->type == DNS_QUERY_A) {
				dprintf("Query result A\n");
				uint32_t result = 0;
				if (dns_collect_request(inbound_id, (char*)&result, sizeof(uint32_t))) {
					dprintf("dns A result collected: %08x\n", result);
					request->callback_a(result, (const char*)request->orig, inbound_id);
				}
			}  else if (request->callback_aaaa && request->type == DNS_QUERY_AAAA) {
				dprintf("Query result AAAA\n");
				uint8_t result[16] = { 0 };
				if (dns_collect_request(inbound_id, (char*)&result, 16)) {
					dprintf("dns AAAA result collected\n");
					request->callback_aaaa(result, (const char*)request->orig, inbound_id);
				}
			} else if (request->callback_ptr && request->type == DNS_QUERY_PTR4) {
				dprintf("Query result PTR4\n");
				char result[256] = { 0 };
				if (dns_collect_request(inbound_id, (char*)&result, sizeof(result))) {
					dprintf("dns PTR4 result collected: %s\n", result);
					request->callback_ptr(result, 0, inbound_id);
				}
			}
		}
		return;
	}
	dprintf("DNS reply id=%d, but we don't have a pending request with this id!\n", inbound_id);
}

static void fill_resource_record(resource_record_t* rr, const unsigned char *input)
{
	rr->type = (uint16_t)((input[0] << 8) + input[1]);
	rr->rr_class = (input[2] << 8) + input[3];
	rr->ttl = (input[4] << 24) + (input[5] << 16) + (input[6] << 8) + input[7];
	rr->rdlength = (input[8] << 8) + input[9];
}

void dns_result_ready(dns_header_t* header, dns_request_t* request, unsigned length, char** error, char* res, uint8_t* outlength)
{
	unsigned i = 0, o;
	int q = 0;
	int curanswer;
 	unsigned short ptr;
	resource_record_t rr;

	error = NULL;
	*res = 0;
	*outlength = 0;

	rr.type = DNS_QUERY_NONE;
	rr.rdlength = 0;
	rr.ttl = 1;
	rr.rr_class = 0;

	header->ancount = ntohs(header->ancount);
	header->qdcount = ntohs(header->qdcount);

	if (!(header->flags1 & FLAGS_MASK_QR)) {
		*error = "Not a query result";
		return;
	} else if (header->flags1 & FLAGS_MASK_OPCODE) {
		*error = "Unexpected value in DNS reply packet";
		return;
	} else if (header->flags2 & FLAGS_MASK_RCODE) {
		*error = "Domain name not found";
		return;
	} else if (header->ancount < 1) {
		*error = "No resource records returned";
		return;
	}

	/* Subtract the length of the header from the length of the packet */
	length -= 12;

	while ((unsigned int)q < header->qdcount && i < length) {
		if (header->payload[i] > 63) {
			i += 6;
			q++;
		} else {
			if (header->payload[i] == 0) {
				q++;
				i += 5;
			} else {
				i += header->payload[i] + 1;
			}
		}
	}

	curanswer = 0;
	while ((unsigned)curanswer < header->ancount) {
		q = 0;
		while (q == 0 && i < length) {
			if (header->payload[i] > 63) {
				i += 2;
				q = 1;
			} else {
				if (header->payload[i] == 0) {
					i++;
					q = 1;
				} else {
					 /* skip length and label */
					i += header->payload[i] + 1;
				}
			}
		}
		if ((int)(length - i) < 10) {
			*error = "Incorrectly sized DNS reply";
			return;
		}

		fill_resource_record(&rr, (const unsigned char*)&header->payload[i]);

		i += 10;
		if (rr.type != request->type) {
			curanswer++;
			i += rr.rdlength;
			continue;
		}
		if (rr.rr_class != request->rr_class) {
			curanswer++;
			i += rr.rdlength;
			continue;
		}
		break;
	}

	if ((unsigned int)curanswer == header->ancount) {
		*error = "No A, AAAA or PTR type answers";
		return;
	}

	if (i + rr.rdlength > (unsigned int)length) {
		*error = "Resource record larger than stated";
		return;
	}

	if (rr.rdlength > 1023) {
		*error = "Resource record too large";
		return;
	}

	request->ttl = rr.ttl;

	switch (rr.type) {
		/*
		 * CNAME and PTR are compressed.  We need to decompress them.
		 */
		case DNS_QUERY_CNAME:
		case DNS_QUERY_PTR: {
			unsigned short lowest_pos = length;
			o = 0;
			q = 0;
			while (q == 0 && i < length && o + 256 < 1023) {
				/* DN label found (byte over 63) */
				if (header->payload[i] > 63) {
					memcpy(&ptr,&header->payload[i],2);

					i = ntohs(ptr);

					/* check that highest two bits are set. if not, we've been had */
					if ((i & DN_COMP_BITMASK) != DN_COMP_BITMASK) {
						*error = "DN label decompression header is bogus";
						return;
					}

					/* mask away the two highest bits. */
					i &= ~DN_COMP_BITMASK;

					/* and decrease length by 12 bytes. */
					i -= 12;

					if (i >= lowest_pos) {
						*error = "Invalid decompression pointer";
						return;
					}
					lowest_pos = i;
				} else {
					if (header->payload[i] == 0) {
						q = 1;
					} else {
						res[o] = 0;
						if (o != 0) {
							res[o++] = '.';
						}

						if (o + header->payload[i] > sizeof(dns_header_t)) {
							*error = "DN label decompression is impossible -- malformed/hostile packet?";
							return;
						}

						memcpy(&res[o], &header->payload[i + 1], header->payload[i]);
						o += header->payload[i];
						i += header->payload[i] + 1;
					}
				}
			}
			res[o++] = 0;
		}
		break;
		case DNS_QUERY_AAAA:
			if (rr.rdlength != 16) {
				*error = "rr.rdlength is larger than 16 bytes for an ipv6 entry -- malformed/hostile packet?";
				return;
			}

			memcpy(res,&header->payload[i],rr.rdlength);
			res[rr.rdlength] = 0;
			o = rr.rdlength;
		break;
		case DNS_QUERY_A:
			if (rr.rdlength != 4) {
				*error = "rr.rdlength is larger than 4 bytes for an ipv4 entry -- malformed/hostile packet?";
				return;
			}

			memcpy(res,&header->payload[i],rr.rdlength);
			res[rr.rdlength] = 0;
			o = rr.rdlength;
		break;
		default:
			*error = "don't know how to handle undefined type -- rejecting";
			return;
		break;
	}
	*outlength = o;
	return;
}

uint8_t dns_collect_request(uint16_t id, char* result, size_t max)
{
	dns_request_t findrequest = { .id = id };
	dns_request_t* request = (dns_request_t*)hashmap_get(dns_replies, &findrequest);
	if (request && request->result_length != 0) {
		uint8_t len = request->result_length;
		memcpy(result, request->result, max > request->result_length ? max : request->result_length);

		if (request->type == DNS_QUERY_A) {
			char ip[16] = { 0 };
			get_ip_str(ip, (const uint8_t*)result);
			dprintf("Cached result '%s' -> '%s'\n", request->orig, ip);
			dns_cache_entry_t cache_entry = {
				.host = strdup((const char *) request->orig),
				.result = strdup(ip),
			};
			hashmap_set(dns_cache, &cache_entry);
		}
		kfree_null(&request->orig);
		hashmap_delete(dns_replies, request);
		return len;
	}
	return 0;
}

bool dns_request_is_completed(uint16_t id)
{
	dns_request_t findrequest = { .id = id };
	dns_request_t* request = (dns_request_t*)hashmap_get(dns_replies, &findrequest);
	return (request && request->result_length);
}

void dns_delete_request(uint16_t id)
{
	dns_request_t findrequest = { .id = id };
	dns_request_t* request = (dns_request_t*)hashmap_get(dns_replies, &findrequest);
	if (request) {
		kfree_null(&request->orig);
		hashmap_delete(dns_replies, request);
	}
}

bool is_cached(uint32_t* ip, const char* hostname)
{
	if (!ip) {
		return false;
	}
	dns_cache_entry_t find_cache = {
		.host = hostname,
	};
	dns_cache_entry_t* cached = (dns_cache_entry_t*)hashmap_get(dns_cache, &find_cache);
	if (cached) {
		dprintf("Returned cached DNS %s -> %s\n", hostname, cached->result);
		*ip = str_to_ip(cached->result);
		return true;
	}
	*ip = 0;
	return false;
}

uint32_t dns_lookup_host_async(uint32_t resolver_ip, const char* hostname, uint32_t* ip, dns_reply_callback_a callback)
{
	dns_request_t request;
	dns_header_t h;
	int length;

	if (ip && is_cached(ip, hostname)) {
		return DNS_RESULT_CACHED;
	};

	if ((length = dns_make_payload(hostname, DNS_QUERY_A, 1, (unsigned char*)&h.payload)) == -1) {
		return 0;
	}

	h.flags1 = FLAGS_MASK_RD;
	h.flags2 = 0;
	h.qdcount = 1;
	h.ancount = h.nscount = h.arcount = 0;
	request.callback_a = callback;
	request.callback_aaaa = NULL;
	request.callback_ptr = NULL;

	dns_send_request(hostname, resolver_ip, &request, &h, length, DNS_QUERY_A);

	return h.id;

}

uint32_t dns_blocking_wait_for_result(uint32_t request_id, uint32_t timeout_ms)
{
	uint32_t result = 0;
	if (timeout_ms == 0) {
		timeout_ms = 5000;
	}
	timeout_ms /= 10; // Timer is only 1 centi-second resolution, not 1ms
	time_t now = get_ticks();

	while (!dns_request_is_completed(request_id)) {
		__asm__ volatile("hlt");
		if (get_ticks() - now > timeout_ms) {
			/* Request timed out */
			dns_delete_request(request_id);
			return 0;
		}
	}
	if (dns_collect_request(request_id, (char*)&result, sizeof(uint32_t))) {
		return result;
	}

	return 0;
}

uint32_t dns_lookup_host(uint32_t resolver_ip, const char* hostname, uint32_t timeout_ms)
{
	uint32_t ip = 0, request_id = dns_lookup_host_async(resolver_ip, hostname, &ip,  NULL);
	if (request_id == DNS_RESULT_CACHED) {
		return ip;
	}
	return dns_blocking_wait_for_result(request_id, timeout_ms);
}

void init_dns()
{
	dns_replies = hashmap_new(sizeof(dns_request_t), 0, 6453563734, 7645356235, dns_request_hash, dns_request_compare, NULL, NULL);
	dns_cache = hashmap_new(sizeof(dns_cache_entry_t), 0, 6453563734, 7645356235, dns_cache_hash, dns_cache_compare, NULL, NULL);
	/* Let the IP stack decide on the port number to use */
	dns_query_port = udp_register_daemon(0, &dns_handle_packet, NULL);
	if (dns_query_port == 0) {
		kprintf("Could not bind DNS port! DNS queries will not resolve.\n");
	}
}
