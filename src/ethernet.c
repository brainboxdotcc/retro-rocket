#include <kernel.h>

int ethernet_send_packet(uint8_t * dst_mac_addr, uint8_t * data, int len, uint16_t protocol) {
    uint8_t src_mac_addr[6];
    ethernet_frame_t * frame = kmalloc(sizeof(ethernet_frame_t) + len);
    void * frame_data = (void*)frame + sizeof(ethernet_frame_t);

    get_mac_addr(src_mac_addr);
    memcpy(frame->src_mac_addr, src_mac_addr, 6);
    memcpy(frame->dst_mac_addr, dst_mac_addr, 6);
    memcpy(frame_data, data, len);
    frame->type = htons(protocol);
    dprintf("ethernet: net driver send packet\n");
    rtl8139_send_packet(frame, sizeof(ethernet_frame_t) + len);
    kfree(frame);
    return len;
}

void ethernet_handle_packet(ethernet_frame_t * packet, int len) {
	void * data = (void*) packet + sizeof(ethernet_frame_t);
	int data_len = len - sizeof(ethernet_frame_t);

	dprintf("ethernet_handle_packet type %04x len %d, data_len %d\n", data_len ? ntohs(packet->type) : 0, len, data_len);

	if (len <= 0) {
		dprintf("Ethernet handler got packet of <0 size");
		return;
	}

	if (ntohs(packet->type) == ETHERNET_TYPE_ARP) {
		// ARP packet
		dprintf("Passing to ARP\n");
		arp_handle_packet((arp_packet_t*)data, data_len);
	} else if (ntohs(packet->type) == ETHERNET_TYPE_IP) {
		// IP packet
		dprintf("Passing to IP\n");
		ip_handle_packet((ip_packet_t*)data);
	} else {
		dprintf("Unknown packet type %d\n", packet->type);
		dump_hex(data, data_len);
	}
}

