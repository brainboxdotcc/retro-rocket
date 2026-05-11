#include <kernel.h>

ethernet_protocol_t* protocol_handlers = NULL;
spinlock_t ethernet_lock = 0;

#define ETHERNET_MAX_FRAME (65536 - sizeof(ethernet_frame_t))

int ethernet_send_packet(const uint8_t* dst_mac_addr, const uint8_t* data, size_t len, uint16_t protocol) {
	if (!dst_mac_addr) {
		return 0;
	}
	if (len + sizeof(ethernet_frame_t) > ETHERNET_MAX_FRAME) {
		return 0;
	}
	uint8_t src_mac_addr[6];
	netdev_t* dev = get_active_network_device();
	uint64_t flags;
	if (!dev) {
		dprintf("no active network device, not sending packet\n");
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
	dev->send_packet(frame, sizeof(ethernet_frame_t) + len);
	unlock_spinlock_irq(&ethernet_lock, flags);
	return len;
}

void ethernet_handle_packet(ethernet_frame_t* packet, size_t len) {
	if (!packet) {
		return;
	}

	if (len < sizeof(ethernet_frame_t) || len > ETHERNET_MAX_FRAME) {
		dprintf("Ethernet handler got runt frame\n");
		return;
	}

	void *data = (void*)packet + sizeof(ethernet_frame_t);
	int data_len = len - (int)sizeof(ethernet_frame_t);

	if (!protocol_handlers) {
		dprintf("No handlers yet but received a packet\n");
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
	if (!handler) {
		return false;
	}
	if (protocol_handlers == NULL) {
		protocol_handlers = kcalloc(UINT16_MAX + 1, sizeof(void*));
		if (!protocol_handlers) {
			return false;
		}
	}
	if (protocol_handlers[protocol_number] == NULL) {
		protocol_handlers[protocol_number] = handler;
		dprintf("Ethernet protocol %04X registered\n", protocol_number);
		return true;
	}
	dprintf("Ethernet protocol %04X already registered!\n", protocol_number);
	return false;
}
