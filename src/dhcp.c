#include <kernel.h>

void dhcp_discover() {
	uint8_t request_ip[4];
	uint8_t dst_ip[4];
	memset(request_ip, 0x0, 4);
	memset(dst_ip, 0xff, 4);
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	kprintf("Configuring network via DHCP\n");
	memset(packet, 0, sizeof(dhcp_packet_t));
	size_t optsize = make_dhcp_packet(packet, DHCPDISCOVER, request_ip, DHCP_TRANSACTION_IDENTIFIER, 0);
	udp_send_packet(dst_ip, 68, 67, packet, optsize);
}

void dhcp_request(uint8_t * request_ip, uint32_t xid, uint32_t server_ip) {
	uint8_t dst_ip[4];
	memset(dst_ip, 0xff, 4);
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	memset(packet, 0, sizeof(dhcp_packet_t));
	//kprintf("dhcp request with type 3, server_ip %08x\n", server_ip);
	size_t optsize = make_dhcp_packet(packet, DHCPREQUEST, request_ip, xid, server_ip);
	udp_send_packet(dst_ip, 68, 67, packet, optsize);
}

void dhcp_handle_packet(dhcp_packet_t* packet, size_t length) {
	if(packet->op == DHCP_REPLY) {
		uint8_t* type = get_dhcp_options(packet, OPT_TYPE);
		if (*type == DHCPOFFER) {
			uint32_t* server_ip = get_dhcp_options(packet, OPT_SERVER_IP);
			if (!server_ip) {
				return;
			}
			dhcp_request((uint8_t*)&packet->your_ip, packet->xid, *server_ip);
			kfree(server_ip);
		} else if (*type == DHCPACK) {
			sethostaddr((const unsigned char*)&packet->your_ip);
			uint32_t* dns = get_dhcp_options(packet, OPT_DNS);
			uint32_t* gateway = get_dhcp_options(packet, OPT_GATEWAY);
			if (dns) {
				//kprintf("DNS: %08x\n", *dns);
				setdnsaddr(*dns);
				kfree(dns);
			}
			if (gateway) {
				//kprintf("GW: %08x\n", *gateway);
				setgatewayaddr(*gateway);
				kfree(gateway);
			}
		} else if (*type == DHCPNAK) {
			/* Negative ack, to be implemented */
		}
		kfree(type);
	}
}

void* get_dhcp_options(dhcp_packet_t* packet, uint8_t type) {
	uint8_t* options = packet->options + 4;
	uint8_t curr_type = *options;
	while (curr_type != 0xff && options < (uint64_t)packet + sizeof(dhcp_packet_t)) {
		curr_type = *options;
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

size_t make_dhcp_packet(dhcp_packet_t * packet, uint8_t msg_type, uint8_t * request_ip, uint32_t xid, uint32_t server_ip) {
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

	*(options++) = OPT_TYPE;
	*(options++) = 1;
	*(options++) = msg_type;

	if (server_ip) {
		// Server identifier
		*(options++) = OPT_SERVER_IP;
		*(options++) = 4;
		*((uint32_t*)(options)) = server_ip;
		options += 4;
	}

	// Client identifier
	*(options++) = OPT_CLIENT_MAC;
	*(options++) = 0x07;
	*(options++) = 0x01;
	get_mac_addr(options);
	options += 6;

	// Requested IP address
	*(options++) = OPT_REQUESTED_IP;
	*(options++) = 0x04;
	*((uint32_t*)(options)) = htonl(0x0a00020e);
	memcpy((uint32_t*)(options), request_ip, 4);
	options += 4;

	// Host Name
	const char* hostname = "retrorocket";
	uint8_t len = strlen(hostname);
	*(options++) = OPT_HOSTNAME;
	*(options++) = len + 1; // len
	memcpy(options, hostname, len);
	options += len;
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

	// END
	*(options++) = 0xff;

	return (size_t)options - (size_t)packet;

}