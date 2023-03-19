#include <kernel.h>

static udp_daemon_handler daemons[USHRT_MAX] = { 0 };

uint16_t udp_calculate_checksum(udp_packet_t * packet) {
	// UDP checksum is optional in IPv4
	return 0;
}

void udp_send_packet(uint8_t * dst_ip, uint16_t src_port, uint16_t dst_port, void * data, int len) {
	int length = sizeof(udp_packet_t) + len;
	udp_packet_t * packet = kmalloc(length);
	memset(packet, 0, sizeof(udp_packet_t));
	packet->src_port = htons(src_port);
	packet->dst_port = htons(dst_port);
	packet->length = htons(length);
	packet->checksum = udp_calculate_checksum(packet);

	// Copy data over
	memcpy((void*)packet + sizeof(udp_packet_t), data, len);
	ip_send_packet(dst_ip, packet, length);
	//DumpHex((unsigned char*)packet, sizeof(udp_packet_t) + len);
}

void udp_handle_packet(udp_packet_t* packet, size_t len) {
	//uint16_t src_port = ntohs(packet->src_port);
	uint16_t dst_port = ntohs(packet->dst_port);
	uint16_t length = ntohs(packet->length);

	void * data_ptr = (void*)packet + sizeof(udp_packet_t);
	uint32_t data_len = length;
	//DumpHex(packet, length + sizeof(udp_packet_t));

	if (daemons[dst_port] != NULL) {
		daemons[dst_port](dst_port, data_ptr, data_len);
	}
}

void udp_register_daemon(uint16_t dst_port, udp_daemon_handler handler) {
	if (daemons[dst_port] != NULL && daemons[dst_port] != handler) {
		kprintf("*** BUG *** udp_register_daemon(%d) called twice!\n", dst_port);
		return;
	}
	daemons[dst_port] = handler;

}