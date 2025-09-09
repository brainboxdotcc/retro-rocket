#include <kernel.h>

static arp_table_entry_t arp_table[512] = {};
static uint8_t zero_hardware_addr[6] = {0, 0, 0, 0, 0, 0};
static int arp_table_size = 0;
static int arp_table_curr = 0;

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

/**
 * @brief Handle an ARP packet, passed from the ethernet driver
 * 
 * @param arp_packet raw ARP packat
 * @param len ARP packet length
 */
void arp_handle_packet(arp_packet_t* arp_packet, [[maybe_unused]] int len) {
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
		if (memcmp(arp_packet->dst_protocol_addr, &my_ip, 4) == 0) {

			netdev_t* dev = get_active_network_device();
			if (!dev) {
				dprintf("No active net dev (arp handle packet)\n");
				return;
			}

			dev->get_mac_addr(arp_packet->src_hardware_addr);
			arp_packet->src_protocol_addr[0] = addr[0];
			arp_packet->src_protocol_addr[1] = addr[1];
			arp_packet->src_protocol_addr[2] = addr[2];
			arp_packet->src_protocol_addr[3] = addr[3];

			memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
			memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

			add_random_entropy((uint64_t)(uint64_t*)arp_packet->dst_hardware_addr);

			arp_packet->opcode = htons(ARP_REPLY);
			arp_packet->hardware_addr_len = 6;
			arp_packet->protocol_addr_len = 4;
			arp_packet->hardware_type = htons(HARDWARE_TYPE_ETHERNET);
			arp_packet->protocol = htons(ETHERNET_TYPE_IP);

			ethernet_send_packet(dst_hardware_addr, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
		}
	} else if(ntohs(arp_packet->opcode) == ARP_REPLY) {
		dprintf("ARP_REPLY from: %08x hw %02x:%02x:%02x:%02x:%02x:%02x\n", *(uint32_t*)&dst_protocol_addr, dst_hardware_addr[0], dst_hardware_addr[1], dst_hardware_addr[2], dst_hardware_addr[3], dst_hardware_addr[4], dst_hardware_addr[5]);
	}

	uint8_t dummy[6];
	if (!arp_lookup(dummy, dst_protocol_addr)) {
		memcpy(&arp_table[arp_table_curr].ip_addr, dst_protocol_addr, 4);
		memcpy(&arp_table[arp_table_curr++].mac_addr, dst_hardware_addr, 6);
		if(arp_table_size < 512) {
			arp_table_size++;
		}
		if(arp_table_curr >= 512) {
			arp_table_curr = 0;
		}
	}

}

void arp_send_packet(uint8_t* dst_hardware_addr, uint8_t* dst_protocol_addr) {
	netdev_t* dev = get_active_network_device();
	if (!dev) {
		dprintf("arp send packet: no active net dev\n");
		return;
	}
	static arp_packet_t * arp_packet = NULL;
	if (!arp_packet) {
		if (!(arp_packet = kmalloc(sizeof(arp_packet_t)))) {
			return;
		}
	}
	static const char broadcast_ip_address[4] = { 255, 255, 255, 255 };

	dev->get_mac_addr(arp_packet->src_hardware_addr);
	if (!gethostaddr(arp_packet->src_protocol_addr)) {
		memcpy(&arp_packet->src_protocol_addr, &broadcast_ip_address, 4);
	}
	memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
	memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);
	arp_packet->opcode = htons(ARP_REQUEST);
	arp_packet->hardware_addr_len = 6;
	arp_packet->protocol_addr_len = 4;
	arp_packet->hardware_type = htons(HARDWARE_TYPE_ETHERNET);
	arp_packet->protocol = htons(ETHERNET_TYPE_IP);

	dprintf("arp send (broadcast) for %08x\n", *(uint32_t*)&dst_protocol_addr);
	ethernet_send_packet(broadcast_mac_address, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
}

void arp_prediscover(uint8_t* protocol_addr) {
	arp_send_packet(zero_hardware_addr, protocol_addr);
}

void arp_lookup_add(uint8_t* ret_hardware_addr, uint8_t* ip_addr) {
	memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);
	memcpy(&arp_table[arp_table_curr].mac_addr, ret_hardware_addr, 6);
	if (arp_table_size < 512) {
		arp_table_size++;
	}
	if (arp_table_curr >= 512) {
		arp_table_curr = 0;
	}
}

int arp_lookup(uint8_t * ret_hardware_addr, uint8_t * ip_addr) {

	/* Special case for broadcast address. Always automatically resolves to 255.255.255.255.
	 */
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
	ethernet_register_iee802_number(ETHERNET_TYPE_ARP, (ethernet_protocol_t)arp_handle_packet);
}

void arp_announce_my_ip(void) {
	uint8_t my_ip[4];
	if (!gethostaddr(my_ip)) {
		return;
	}
	arp_prediscover(my_ip);
}
