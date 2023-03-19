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

void ip_send_packet(uint8_t * dst_ip, void * data, int len) {
	int arp_sent = 3;
	ip_packet_t * packet = kmalloc(sizeof(ip_packet_t) + len);
	memset(packet, 0, sizeof(ip_packet_t));
	packet->version = IP_IPV4;
	packet->ihl = 5; // 5 * 4 = 20 byte
	packet->tos = 0; // Don't care
	packet->length = sizeof(ip_packet_t) + len;
	packet->id = 0; // Used for ip fragmentation, use later
	// Tell router to not divide the packet, and this is packet is the last piece of the fragments.
	packet->frag.bits = 0;
	packet->ttl = 64;
	// XXX: This is hard coded until other protocols are supported.
	packet->protocol = PROTOCOL_UDP;

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
}


void ip_handle_packet(ip_packet_t * packet) {
	char src_ip[20];
	*((uint8_t*)(&packet->version_ihl_ptr)) = ntohb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
	*((uint8_t*)(packet->flags_fragment_ptr)) = ntohb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
	if(packet->version == IP_IPV4) {
		get_ip_str(src_ip, packet->src_ip);
		void * data_ptr = (void*)packet + packet->ihl * 4;
		size_t data_len = ntohs(packet->length) - sizeof(ip_packet_t);

		// https://broken-code.medium.com/fragmentation-933e780b98a3
		/* For fragmented packets, we need to store up the fragments into an ordered list, sorted by frag offset.
		 * Once we receive the very last fragment, we can deliver it all as one. We need to also check the id of the
		 * fragmented packet is identical to the id of other fragments, as may receive different packets fragments
		 * intermixed, and we don't want to accidentally put them into the wrong fragment lists!
		 */
		uint16_t frag_offset = ((uint16_t)packet->frag.fragment_offset_low | ((uint16_t)packet->frag.fragment_offset_high << 8));
		if (packet->frag.more_fragments_follow) {
			/* Packet is part of a fragmented set */
			if (frag_offset == 0) {
				/* First fragment */
				kprintf("*** WARN *** Fragmented IP (first frag offset 0), and we don't support this yet :(");
			} else {
				/* Middle fragments */
				kprintf("*** WARN *** Fragmented IP (middle frag offset %d), and we don't support this yet :(", frag_offset);
			}
			return;
		} else if (packet->frag.more_fragments_follow == 0 && (frag_offset != 0)) {
			/* Final fragment of fragmented set.
			 * Once we get this fragment, we can deliver the reassembled packet.
			 */
			kprintf("*** WARN *** Fragmented IP (last frag offset %d), and we don't support this yet :(", frag_offset);
			return;
		}

		if (packet->protocol == PROTOCOL_ICMP) {
			kprintf("ICMP packet received\n");
		} else if (packet->protocol == PROTOCOL_UDP) {
			udp_handle_packet(data_ptr, data_len);
		} else if (packet->protocol == PROTOCOL_TCP) {
		}
	}
}