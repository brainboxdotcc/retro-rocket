#include <kernel.h>

arp_table_entry_t arp_table[512];
int arp_table_size = 0;
int arp_table_curr = 0;

arp_table_entry_t* get_arp_entry(size_t index) {
	if (index > 512) {
		return NULL;
	}
	return &arp_table[index];
}

size_t get_arp_table_size() {
	return arp_table_size;
}

uint8_t broadcast_mac_address[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void arp_handle_packet(arp_packet_t* arp_packet, int len) {
	unsigned char dst_hardware_addr[6];
	unsigned char dst_protocol_addr[4];
	memcpy(dst_hardware_addr, arp_packet->src_hardware_addr, 6);
	memcpy(dst_protocol_addr, arp_packet->src_protocol_addr, 4);
	if (ntohs(arp_packet->opcode) == ARP_REQUEST) {
		unsigned char addr[4];
		uint32_t my_ip = 0;
		if (gethostaddr(addr)) {
			my_ip = *(uint32_t*)addr;
		}
		if (memcmp(arp_packet->dst_protocol_addr, &my_ip, 4)) {

			get_mac_addr(arp_packet->src_hardware_addr);
			arp_packet->src_protocol_addr[0] = addr[0];
			arp_packet->src_protocol_addr[1] = addr[1];
			arp_packet->src_protocol_addr[2] = addr[2];
			arp_packet->src_protocol_addr[3] = addr[3];

			memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
			memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

			arp_packet->opcode = htons(ARP_REPLY);
			arp_packet->hardware_addr_len = 6;
			arp_packet->protocol_addr_len = 4;
			arp_packet->hardware_type = htons(HARDWARE_TYPE_ETHERNET);
			arp_packet->protocol = htons(ETHERNET_TYPE_IP);

			ethernet_send_packet(dst_hardware_addr, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
		}
	}
	else if(ntohs(arp_packet->opcode) == ARP_REPLY) {
	}

	memcpy(&arp_table[arp_table_curr].ip_addr, dst_protocol_addr, 4);
	memcpy(&arp_table[arp_table_curr].mac_addr, dst_hardware_addr, 6);
	if(arp_table_size < 512) {
		arp_table_size++;
	}
	if(arp_table_curr >= 512) {
		arp_table_curr = 0;
	}

}

void arp_send_packet(uint8_t* dst_hardware_addr, uint8_t* dst_protocol_addr) {
	arp_packet_t * arp_packet = kmalloc(sizeof(arp_packet_t));

	get_mac_addr(arp_packet->src_hardware_addr);
	arp_packet->src_protocol_addr[0] = 10;
	arp_packet->src_protocol_addr[1] = 0;
	arp_packet->src_protocol_addr[2] = 2;
	arp_packet->src_protocol_addr[3] = 14;

	memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
	memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);
	arp_packet->opcode = htons(ARP_REQUEST);
	arp_packet->hardware_addr_len = 6;
	arp_packet->protocol_addr_len = 4;
	arp_packet->hardware_type = htons(HARDWARE_TYPE_ETHERNET);
	arp_packet->protocol = htons(ETHERNET_TYPE_IP);

	ethernet_send_packet(broadcast_mac_address, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
}

void arp_lookup_add(uint8_t* ret_hardware_addr, uint8_t* ip_addr) {
	memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);
	memcpy(&arp_table[arp_table_curr].mac_addr, ret_hardware_addr, 6);
	if(arp_table_size < 512) {
		arp_table_size++;
	}
	if(arp_table_curr >= 512) {
		arp_table_curr = 0;
	}
}

int arp_lookup(uint8_t * ret_hardware_addr, uint8_t * ip_addr) {

	if (*((uint32_t*)ip_addr) == 0xffffffff) {
		memcpy(ret_hardware_addr, broadcast_mac_address, 6);
		return 1;
	}

	uint32_t ip_entry = *((uint32_t*)(ip_addr));
	for(int i = 0; i < 512; i++) {
		if(arp_table[i].ip_addr == ip_entry) {
			memcpy(ret_hardware_addr, &arp_table[i].mac_addr, 6);
			return 1;
		}
	}
	return 0;
}

void arp_init() {
	uint8_t broadcast_ip[4];
	uint8_t broadcast_mac[6];

	memset(broadcast_ip, 0xff, 4);
	memset(broadcast_mac, 0xff, 6);
	arp_lookup_add(broadcast_mac, broadcast_ip);
}