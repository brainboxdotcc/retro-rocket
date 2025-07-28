#include <kernel.h>

ethernet_protocol_t* protocol_handlers = NULL;

int ethernet_send_packet(uint8_t* dst_mac_addr, uint8_t* data, int len, uint16_t protocol) {
	uint8_t src_mac_addr[6];
	netdev_t* dev = get_active_network_device();
	if (!dev) {
		return 0;
	}

	ethernet_frame_t * frame = kmalloc(sizeof(ethernet_frame_t) + len);
	if (!frame) {
		return 0;
	}
	void * frame_data = (void*)frame + sizeof(ethernet_frame_t);

	dev->get_mac_addr(src_mac_addr);
	memcpy(frame->src_mac_addr, src_mac_addr, 6);
	memcpy(frame->dst_mac_addr, dst_mac_addr, 6);
	memcpy(frame_data, data, len);
	frame->type = htons(protocol);
	dprintf("ethernet_send_packet frame=%08x\n", frame);
	dev->send_packet(frame, sizeof(ethernet_frame_t) + len);
	kfree_null(&frame);
	return len;
}

void ethernet_handle_packet(ethernet_frame_t* packet, int len) {
	void * data = (void*) packet + sizeof(ethernet_frame_t);
	int data_len = len - sizeof(ethernet_frame_t);

	if (len <= 0) {
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
	dprintf("Unknown packet type %d\n", packet->type);
}

bool ethernet_register_iee802_number(uint16_t protocol_number, ethernet_protocol_t handler)
{
	if (protocol_handlers == NULL) {
		protocol_handlers = kmalloc(sizeof(void*) * UINT16_MAX);
		if (!protocol_handlers) {
			return false;
		}
	}
	if (protocol_handlers[protocol_number] == NULL) {
		protocol_handlers[protocol_number] = handler;
		dprintf("Protocol %04X registered\n", protocol_number);
		return true;
	}
	dprintf("Protocol %04X already registered!\n", protocol_number);
	return false;
}
