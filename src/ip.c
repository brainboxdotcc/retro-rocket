#include <kernel.h>

uint8_t my_ip[] = {10, 0, 2, 14};
uint8_t test_target_ip[] = {10, 0, 2, 15};
uint8_t zero_hardware_addr[] = {0,0,0,0,0,0};

void get_ip_str(char * ip_str, uint8_t * ip) {
	sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

char ip_addr[4];
int is_ip_allocated = 0;
uint32_t dns_addr = 0, gateway_addr = 0;

struct hashmap *frag_map = NULL;

int gethostaddr(unsigned char *addr) {
	if(!is_ip_allocated) {
		return 0;
	}
	memcpy(addr, ip_addr, 4);
	return 1;
}

void sethostaddr(const unsigned char* addr) {
	memcpy(ip_addr, addr, 4);
	is_ip_allocated = 1;
}

void setdnsaddr(uint32_t dns) {
	dns_addr = dns;
}

void setgatewayaddr(uint32_t gateway) {
	gateway_addr = gateway;
}

uint32_t getdnsaddr() {
	return dns_addr;
}

uint32_t getgatewayaddr() {
	return gateway_addr;
}

uint16_t ip_calculate_checksum(ip_packet_t * packet) {
	// Treat the packet header as a 2-byte-integer array
	// Sum all integers switch to network byte order
	int array_size = sizeof(ip_packet_t) / 2;
	uint16_t * array = (uint16_t*)packet; // XXX: Alignment!
	uint32_t sum = 0;
	for(int i = 0; i < array_size; i++) {
		sum += htons(array[i]);
	}
	uint32_t carry = sum >> 16;
	sum = sum & 0x0000ffff;
	sum = sum + carry;
	uint16_t ret = ~sum;
	return ret;
}

void ip_send_packet(uint8_t* dst_ip, void* data, uint16_t len, uint8_t protocol) {
	int arp_sent = 3;
	ip_packet_t * packet = kmalloc(sizeof(ip_packet_t) + len);
	memset(packet, 0, sizeof(ip_packet_t));
	packet->version = IP_IPV4;
	packet->ihl = sizeof(ip_packet_t) / sizeof(uint32_t); // header length is in 32 bit words
	packet->tos.bits = 0; // Don't care
	packet->length = sizeof(ip_packet_t) + len;
	packet->id = 0; // Used for ip fragmentation, use later
	// Tell router to not divide the packet, and this is packet is the last piece of the fragments.
	packet->frag.bits = 0;
	packet->ttl = 64;
	// XXX: This is hard coded until other protocols are supported.
	packet->protocol = protocol;

	gethostaddr(my_ip);
	memcpy(packet->src_ip, my_ip, 4);
	memcpy(packet->dst_ip, dst_ip, 4);
	void * packet_data = (void*)packet + packet->ihl * 4;
	memcpy(packet_data, data, len);
	*((uint8_t*)(&packet->version_ihl_ptr)) = htonb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
	*((uint8_t*)(packet->flags_fragment_ptr)) = htonb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
	packet->length = htons(sizeof(ip_packet_t) + len);
	packet->header_checksum = 0;
	packet->header_checksum = htons(ip_calculate_checksum(packet));
	uint8_t dst_hardware_addr[6];
	// Attempt to resolve ARP or find in arp cache table
	while (!arp_lookup(dst_hardware_addr, dst_ip)) {
		if(arp_sent != 0) {
			arp_sent--;
		}
		arp_send_packet(zero_hardware_addr, dst_ip);
	}
	ethernet_send_packet(dst_hardware_addr, (uint8_t*)packet, htons(packet->length), ETHERNET_TYPE_IP);
	// Remember to free the packet!
	kfree(packet);
}

ip_packet_frag_t* frag_list_insert(ip_packet_frag_t *insert, ip_packet_frag_t *list)
{
	ip_packet_frag_t *tmp = list, *tmp2;
	if (!tmp) {
		insert->prev = 0;
		insert->next = 0;
		return insert;
	}
	if (tmp->offset >= insert->offset) {	
		tmp->prev = insert;
		insert->next = tmp;
		insert->prev = 0;
		return insert;
	}
	tmp2 = tmp;
	tmp = tmp->next;
	while (tmp) {
		if (tmp->offset >= insert->offset) {
			tmp->prev = insert;
			insert->next = tmp;
			tmp2->next = insert;
			insert->prev = tmp2;
			return list;
		}
		tmp2 = tmp;
		tmp = tmp->next;
	}
	insert->next = 0;
	tmp2->next = insert;
	insert->prev = tmp2;
	return list;	
}

int ip_frag_compare(const void *a, const void *b, void *udata) {
    const ip_fragmented_packet_parts_t* fa = a;
    const ip_fragmented_packet_parts_t* fb = b;
    return fa->id == fb->id ? 0 : (fa->id < fb->id ? -1 : 1);
}

uint64_t ip_frag_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const ip_fragmented_packet_parts_t* frag_parts = item;
    return hashmap_murmur(&(frag_parts->id), sizeof(frag_parts->id), seed0, seed1);
}

void ip_handle_packet(ip_packet_t * packet) {
	char src_ip[20];
	*((uint8_t*)(&packet->version_ihl_ptr)) = ntohb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
	*((uint8_t*)(packet->flags_fragment_ptr)) = ntohb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
	if(packet->version == IP_IPV4) {
		get_ip_str(src_ip, packet->src_ip);
		void * data_ptr = (void*)packet + packet->ihl * 4;
		size_t data_len = ntohs(packet->length) - (packet->ihl * 4);
		bool fragment_to_free = false;

		/* For fragmented packets, we store up the fragments into an ordered list, sorted by frag offset.
		 * Once we receive the very last fragment, we can deliver it all as one. We store each list of packet
		 * waiting to be reassembled into a hashmap keyed by id, so they can be accessed in O(n) time.
		 * 
		 * BUG, FIXME: If we never receive the ending fragment for a fragmented group of packets, the memory
		 * used stays used! This could be abused, and must be fixed:
		 * https://en.wikipedia.org/wiki/IP_fragmentation_attack#Exploits
		 */
		uint16_t frag_offset = ((uint16_t)packet->frag.fragment_offset_low | ((uint16_t)packet->frag.fragment_offset_high << 8));
		if (!packet->frag.dont_fragment) {
			if (packet->frag.more_fragments_follow) {
				/* Packet is part of a fragmented set */
				if (frag_offset == 0) {
					/* First fragment */
					if (frag_map == NULL) {
						frag_map = hashmap_new(sizeof(ip_fragmented_packet_parts_t), 0, 0, 0, ip_frag_hash, ip_frag_compare, NULL, NULL);
					}
					ip_fragmented_packet_parts_t fragmented;
					ip_packet_frag_t* fragment = (ip_packet_frag_t*)kmalloc(sizeof(ip_packet_frag_t*));
					fragmented.id = packet->id;
					fragmented.ordered_list += data_len;
					fragmented.size = 0;
					fragmented.ordered_list = NULL;
					fragment->offset = frag_offset;
					fragment->packet = (ip_packet_t*)kmalloc(ntohs(packet->length));
					memcpy(fragment->packet, packet, ntohs(packet->length));
					frag_list_insert(fragment, fragmented.ordered_list);
					hashmap_set(frag_map, &fragmented);
				} else {
					ip_packet_t findpacket = { .id = packet->id };
					ip_fragmented_packet_parts_t* fragmented = (ip_fragmented_packet_parts_t*)hashmap_get(frag_map, &findpacket);
					if (fragmented == NULL) {
						kprintf("*** WARN *** Fragmented packet id %d has no entry in hash map", fragmented);
						return;
					}
					ip_packet_frag_t* fragment = (ip_packet_frag_t*)kmalloc(sizeof(ip_packet_frag_t*));
					fragmented->size += data_len;
					fragment->offset = frag_offset;
					fragment->packet = (ip_packet_t*)kmalloc(ntohs(packet->length));
					memcpy(fragment->packet, packet, ntohs(packet->length));
					frag_list_insert(fragment, fragmented->ordered_list);
				}
				return;
			} else if (packet->frag.more_fragments_follow == 0 && (frag_offset != 0)) {
				/* Final fragment of fragmented set.
				* Once we get this fragment, we can deliver the reassembled packet.
				*/
				ip_packet_t findpacket = { .id = packet->id };
				ip_fragmented_packet_parts_t* fragmented = (ip_fragmented_packet_parts_t*)hashmap_get(frag_map, &findpacket);
				if (fragmented == NULL) {
					kprintf("*** WARN *** Fragmented packet id %d has no entry in hash map", fragmented);
					return;
				}
				ip_packet_frag_t* fragment = (ip_packet_frag_t*)kmalloc(sizeof(ip_packet_frag_t*));
				fragmented->size += data_len;
				fragment->offset = frag_offset;
				fragment->packet = (ip_packet_t*)kmalloc(ntohs(packet->length));
				memcpy(fragment->packet, packet, ntohs(packet->length));
				frag_list_insert(fragment, fragmented->ordered_list);

				/* We have complete packet, all fragments - reassemble the data part and free everything */
				ip_packet_frag_t* cur = fragmented->ordered_list;
				data_ptr = kmalloc(fragmented->size);
				data_len = fragmented->size;
				fragment_to_free = true;
				for (; cur; cur = cur->next) {
					size_t this_packet_size = ntohs(cur->packet->length) - (cur->packet->ihl * 4);
					if (cur->offset + this_packet_size < data_len) {
						void * copy_from = (void*)cur->packet + cur->packet->ihl * 4;
						memcpy(data_ptr + cur->offset, copy_from, this_packet_size);
					} else {
						//kprintf("*** WARN *** Fragmented packet id %d has fragment with offset %08x and length %08d >= data length of %08x", fragmented->id, cur->offset, this_packet_size, data_len);
					}
					kfree(cur->packet);
					kfree(cur);
				}
				hashmap_delete(frag_map, &findpacket);
				// Now we have reassembled the data portion, we can fall through and let the packet be handled...
			}
		}

		if (packet->protocol == PROTOCOL_ICMP) {
			kprintf("ICMP packet received\n");
		} else if (packet->protocol == PROTOCOL_UDP) {
			udp_handle_packet(packet, data_ptr, data_len);
		} else if (packet->protocol == PROTOCOL_TCP) {
		}

		if (fragment_to_free) {
			kfree(data_ptr);
		}
	}
}