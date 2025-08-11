#include <kernel.h>

ethernet_protocol_t* protocol_handlers = NULL;
spinlock_t ethernet_lock = 0;

#define ETHERNET_MAX_FRAME (65536 + sizeof(ethernet_frame_t))

int ethernet_send_packet(uint8_t* dst_mac_addr, uint8_t* data, uint32_t len, uint16_t protocol) {
	if (len + sizeof(ethernet_frame_t) > ETHERNET_MAX_FRAME) {
		return 0;
	}
	uint8_t src_mac_addr[6];
	netdev_t* dev = get_active_network_device();
	uint64_t flags;
	if (!dev) {
		return 0;
	}
	lock_spinlock_irq(&ethernet_lock, &flags);

	static ethernet_frame_t * frame = NULL;
	if (frame == NULL) {
		frame = kmalloc(ETHERNET_MAX_FRAME);
	}
	if (!frame) {
		unlock_spinlock_irq(&ethernet_lock, flags);
		return 0;
	}
	void * frame_data = (void*)frame + sizeof(ethernet_frame_t);

	dev->get_mac_addr(src_mac_addr);
	memcpy(frame->src_mac_addr, src_mac_addr, 6);
	memcpy(frame->dst_mac_addr, dst_mac_addr, 6);
	memcpy(frame_data, data, len);
	frame->type = htons(protocol);
	dprintf("ethernet_send_packet frame=%08lx\n", (uint64_t)frame);
	dev->send_packet(frame, sizeof(ethernet_frame_t) + len);
	unlock_spinlock_irq(&ethernet_lock, flags);
	return len;
}

void ethernet_handle_packet(ethernet_frame_t* packet, int len) {
	void * data = (void*) packet + sizeof(ethernet_frame_t);
	int data_len = len - (int)sizeof(ethernet_frame_t);

	if (len < 0) {
		dprintf("Ethernet handler got packet of <0 size");
		return;
	}

	uint16_t packet_type = ntohs(packet->type);
	ethernet_protocol_t handler = protocol_handlers[packet_type];
	if (data_len >= 8) {
		add_random_entropy(*(uint64_t*)data);		
	}
	if (handler != NULL) {
		handler(data, data_len);
		return;
	}
}

bool ethernet_register_iee802_number(uint16_t protocol_number, ethernet_protocol_t handler)
{
	if (protocol_handlers == NULL) {
		protocol_handlers = kmalloc(sizeof(void*) * (UINT16_MAX + 1));
		if (!protocol_handlers) {
			return false;
		}
		memset(protocol_handlers, 0, sizeof(void*) * (UINT16_MAX + 1));
	}
	if (protocol_handlers[protocol_number] == NULL) {
		protocol_handlers[protocol_number] = handler;
		dprintf("Protocol %04X registered\n", protocol_number);
		return true;
	}
	dprintf("Protocol %04X already registered!\n", protocol_number);
	return false;
}
