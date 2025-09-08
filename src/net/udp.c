#include <kernel.h>

typedef struct {
	udp_daemon_handler func;
	void* opaque;
} udp_daemon_registration;

static udp_daemon_registration daemons[USHRT_MAX] = {};
static spinlock_t udp_lock = 0;
#define UDP_MAX_PACKET (65536 + sizeof(udp_packet_t))

uint16_t udp_calculate_checksum(udp_packet_t * packet) {
	// UDP checksum is optional in IPv4
	return 0;
}

void udp_send_packet(uint8_t * dst_ip, uint16_t src_port, uint16_t dst_port, void * data, uint16_t len) {
	uint64_t flags;
	lock_spinlock_irq(&udp_lock, &flags);
	uint16_t length = sizeof(udp_packet_t) + len;
	uint8_t p[length];
	udp_packet_t* packet = (udp_packet_t*)p;
	memset(packet, 0, sizeof(udp_packet_t));
	packet->src_port = htons(src_port);
	packet->dst_port = htons(dst_port);
	packet->length = htons(length);
	packet->checksum = udp_calculate_checksum(packet);
	// Copy data over
	memcpy((void*)packet + sizeof(udp_packet_t), data, len);
	ip_send_packet(dst_ip, packet, length, PROTOCOL_UDP);
	unlock_spinlock_irq(&udp_lock, flags);
}

void udp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, udp_packet_t* packet, size_t len) {
	uint16_t dst_port = ntohs(packet->dst_port);
	uint16_t src_port = ntohs(packet->src_port);
	uint16_t length = ntohs(packet->length);
	uint32_t src_ip = ntohl(*(uint32_t*)(encap_packet->src_ip));

	void * data_ptr = (void*)packet + sizeof(udp_packet_t);
	uint32_t data_len = length;

	if (daemons[dst_port].func != NULL) {
		daemons[dst_port].func(src_ip, src_port, dst_port, data_ptr, data_len, daemons[dst_port].opaque);
	}
}

bool udp_unregister_daemon(uint16_t dst_port, udp_daemon_handler handler) {
	if (daemons[dst_port].func == handler) {
		daemons[dst_port].func = NULL;
		daemons[dst_port].opaque = NULL;
		return true;
	}
	return false;
}

uint16_t udp_register_daemon(uint16_t dst_port, udp_daemon_handler handler, void* opaque) {

	if (dst_port == 0) {
		/* Let the OS allocate a port
		 * Walk up the port table, looking for one that doesn't have
		 * any daemon bound to it. If we wrap back around to 0, then
		 * there are no free ports remaining to bind to.
		 */
		for(dst_port = 1024; daemons[dst_port].func != NULL && dst_port != 0; ++dst_port);
		if (dst_port == 0) {
			return 0;
		}
	}

	if (daemons[dst_port].func != NULL && daemons[dst_port].func != handler) {
		dprintf("*** BUG *** udp_register_daemon(%d) called twice!\n", dst_port);
		return 0;
	}
	daemons[dst_port].func = handler;
	daemons[dst_port].opaque = opaque;
	return dst_port;
}
