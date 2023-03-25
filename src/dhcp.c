#include <kernel.h>

void dhcp_handle_packet([[maybe_unused]] uint16_t dst_port, void* data, uint32_t length) {
	dhcp_packet_t* packet = (dhcp_packet_t*)data;
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
			uint32_t* subnet = get_dhcp_options(packet, OPT_SUBNET);
			if (subnet) {
				setnetmask(*subnet);
				kfree(subnet);
			}
			if (dns) {
				setdnsaddr(*dns);
				//arp_prediscover((uint8_t*)dns);
				kfree(dns);
			}
			if (gateway) {
				setgatewayaddr(*gateway);
				//arp_prediscover((uint8_t*)gateway);
				kfree(gateway);
			}
			tcp_connect(0x01010101, 80);
		} else if (*type == DHCPNAK) {
			/* Negative ack, to be implemented */
		}
		kfree(type);
	}
}

void dhcp_discover() {
	uint8_t request_ip[4] = { 0, 0, 0, 0 };
	uint8_t dst_ip[4] = { 0xff, 0xff, 0xff, 0xff };
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	udp_register_daemon(DHCP_DST_PORT, &dhcp_handle_packet);
	kprintf("Configuring network via DHCP\n");
	memset(packet, 0, sizeof(dhcp_packet_t));
	uint16_t optsize = make_dhcp_packet(packet, DHCPDISCOVER, request_ip, DHCP_TRANSACTION_IDENTIFIER, 0);
	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, packet, optsize);
}

void dhcp_request(uint8_t* request_ip, uint32_t xid, uint32_t server_ip) {
	uint8_t dst_ip[4] = { 0xff, 0xff, 0xff, 0xff };
	dhcp_packet_t * packet = kmalloc(sizeof(dhcp_packet_t));
	memset(packet, 0, sizeof(dhcp_packet_t));
	//kprintf("dhcp request with type 3, server_ip %08x\n", server_ip);
	uint16_t optsize = make_dhcp_packet(packet, DHCPREQUEST, request_ip, xid, server_ip);
	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, packet, optsize);
}

void* get_dhcp_options(dhcp_packet_t* packet, uint8_t type) {
	uint8_t* options = packet->options + 4;
	uint8_t curr_type = *options;
	while (curr_type != OPT_END && (uint64_t)options < (uint64_t)packet + sizeof(dhcp_packet_t)) {
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

uint16_t make_dhcp_packet(dhcp_packet_t* packet, uint8_t msg_type, uint8_t* request_ip, uint32_t xid, uint32_t server_ip) {
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
	*(options++) = 7;
	*(options++) = HARDWARE_TYPE_ETHERNET;
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
	*(options++) = OPT_PARAMETER_REQUEST_LIST;
	*(options++) = 8;
	*(options++) = OPT_SUBNET;
	*(options++) = OPT_GATEWAY;
	*(options++) = OPT_DNS;
	*(options++) = OPT_DOMAIN;
	*(options++) = OPT_NBNS;
	*(options++) = OPT_NBDD;
	*(options++) = OPT_NETBIOS_NODE_TYPE;
	*(options++) = OPT_MAX_DHCP_SIZE;

	// END
	*(options++) = OPT_END;

	return (uint16_t)((size_t)options - (size_t)packet);
}