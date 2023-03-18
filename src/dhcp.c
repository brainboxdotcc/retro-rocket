#include <kernel.h>

void dhcp_discover() {
	uint8_t request_ip[4];
	uint8_t dst_ip[4];
	memset(request_ip, 0x0, 4);
	memset(dst_ip, 0xff, 4);
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	kprintf("Configuring network via DHCP\n");
	memset(packet, 0, sizeof(dhcp_packet_t));
	make_dhcp_packet(packet, 1, request_ip, DHCP_TRANSACTION_IDENTIFIER, 0);
	udp_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
}

void dhcp_request(uint8_t * request_ip, uint32_t xid, uint32_t server_ip) {
	uint8_t dst_ip[4];
	memset(dst_ip, 0xff, 4);
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	memset(packet, 0, sizeof(dhcp_packet_t));
	make_dhcp_packet(packet, 3, request_ip, xid, server_ip);
	udp_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
}

void dhcp_handle_packet(dhcp_packet_t * packet, size_t length) {
	if(packet->op == DHCP_REPLY) {
		uint8_t * type = get_dhcp_options(packet, 53);
		if(*type == 2) {
			dhcp_request((uint8_t*)&packet->your_ip, packet->xid, packet->server_ip);
			sethostaddr((const unsigned char*)&packet->your_ip);
		}
		else if (*type == 5) {
			sethostaddr((const unsigned char*)&packet->your_ip);
			uint32_t* dns = get_dhcp_options(packet, 6);
			uint32_t* gateway = get_dhcp_options(packet, 3);
			if (dns) {
				setdnsaddr(*dns);
			}
			if (gateway) {
				setgatewayaddr(*gateway);
			}
		}
	}
}

void* get_dhcp_options(dhcp_packet_t* packet, uint8_t type) {
	uint8_t* options = packet->options + 4;
	uint8_t curr_type = *options;
	while (curr_type != 0xff && options < (uint8_t*)packet + sizeof(dhcp_packet_t)) {
		uint8_t len = *(options + 1);
		if (curr_type == type) {
			void* ret = kmalloc(len);
			memcpy(ret, options + 2, len);
			return ret;
		}
		options += (2 + len);
	}
	return NULL;
}

void make_dhcp_packet(dhcp_packet_t * packet, uint8_t msg_type, uint8_t * request_ip, uint32_t xid, uint32_t server_ip) {
	packet->op = DHCP_REQUEST;
	packet->hardware_type = HARDWARE_TYPE_ETHERNET;
	packet->hardware_addr_len = 6;
	packet->hops = 0;
	packet->xid = xid;
	packet->flags = htons(0x8000);
	packet->client_ip = 0;
	packet->server_ip = server_ip;
	packet->your_ip = *((uint32_t*)request_ip);
	get_mac_addr(packet->client_hardware_addr);

	uint8_t dst_ip[4];
	memset(dst_ip, 0xff, 4);

	uint8_t * options = packet->options;
	*((uint32_t*)(options)) = htonl(0x63825363);
	options += 4;

	*(options++) = 53;
	*(options++) = 1;
	*(options++) = msg_type;

	if (server_ip) {
		// Server identifier
		*(options++) = 54;
		*(options++) = 4;
		*(options++) = server_ip;
	}

	// Client identifier
	*(options++) = 61;
	*(options++) = 0x07;
	*(options++) = 0x01;
	get_mac_addr(options);
	options += 6;

	// Requested IP address
	*(options++) = 50;
	*(options++) = 0x04;
	*((uint32_t*)(options)) = htonl(0x0a00020e);
	memcpy((uint32_t*)(options), request_ip, 4);
	options += 4;

	// Host Name
	*(options++) = 12;
	*(options++) = strlen("retrorocket") + 1; // len
	memcpy(options, "retrorocket", strlen("retrorocket"));
	options += strlen("retrorocket");
	*(options++) = 0x00;

	// Parameter request list
	*(options++) = 55;
	*(options++) = 8;
	*(options++) = 0x1;
	*(options++) = 0x3;
	*(options++) = 0x6;
	*(options++) = 0xf;
	*(options++) = 0x2c;
	*(options++) = 0x2e;
	*(options++) = 0x2f;
	*(options++) = 0x39;
	*(options++) = 0xff;

}