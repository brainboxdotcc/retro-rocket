#include <kernel.h>

static uint32_t dhcp_curr_xid = 0;
static uint32_t dhcp_selected_server = 0;   /* network order */
static uint32_t dhcp_server_id = 0;         /* network order */
static volatile int64_t dhcp_t1_ms = -1;
static volatile int64_t dhcp_t2_ms = -1;
static volatile int64_t dhcp_expiry_ms = -1;
static volatile bool dhcp_have_lease = false;
static volatile bool dhcp_renew_sent = false;
static volatile bool dhcp_rebind_sent = false;
extern int is_ip_allocated, is_dns_allocated, is_gateway_allocated, is_mask_allocated;

static void dhcp_background(void);
static void dhcp_request_renew_unicast(void);
static void dhcp_request_rebind_broadcast(void);
static uint16_t make_dhcp_request_ciaddr(dhcp_packet_t* packet, uint32_t xid, uint32_t ciaddr_netorder);

void dhcp_handle_packet(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t length, void* opaque) {
	dhcp_packet_t* packet = (dhcp_packet_t*)data;

	/* Only process replies */
	if (packet->op != DHCP_REPLY) {
		return;
	}

	/* Drop if the reply is not for our MAC */
	netdev_t* dev = get_active_network_device();
	if (dev == NULL) {
		return;
	}

	uint8_t mymac[6];
	dev->get_mac_addr(mymac);

	if (memcmp(packet->client_hardware_addr, mymac, 6) != 0) {
		return;
	}

	/* Drop if this is not for our current exchange */
	if (packet->xid != dhcp_curr_xid) {
		return;
	}

	uint8_t* type = get_dhcp_options(packet, OPT_TYPE);
	if (type == NULL) {
		return;
	}

	if (*type == DHCPOFFER) {
		uint32_t* server_ip = get_dhcp_options(packet, OPT_SERVER_IP);
		if (server_ip == NULL) {
			kfree_null(&type);
			return;
		}

		/* If we've already chosen a server for this xid, ignore competing offers */
		if (dhcp_selected_server != 0 && *server_ip != dhcp_selected_server) {
			kfree_null(&server_ip);
			kfree_null(&type);
			return;
		}

		dhcp_selected_server = *server_ip;

		/* Request the offered address from the selected server */
		dhcp_request((uint8_t*)&packet->your_ip, packet->xid, *server_ip);

		kfree_null(&server_ip);
		kfree_null(&type);
		return;
	}

	if (*type == DHCPACK) {
		/* Only accept ACKs from the server we selected */
		uint32_t* server_ip = get_dhcp_options(packet, OPT_SERVER_IP);
		if (server_ip == NULL) {
			kfree_null(&type);
			return;
		}

		if (*server_ip != dhcp_selected_server) {
			kfree_null(&server_ip);
			kfree_null(&type);
			return;
		}

		/* Commit the lease address */
		if (!is_ip_allocated) {
			sethostaddr((const unsigned char *) &packet->your_ip);
			dprintf("DHCP: sethostaddr(%08x)\n", packet->your_ip);
		}

		/* Optional parameters */
		uint32_t* dns = get_dhcp_options(packet, OPT_DNS);
		uint32_t* gateway = get_dhcp_options(packet, OPT_GATEWAY);
		uint32_t* subnet = get_dhcp_options(packet, OPT_SUBNET);
		uint32_t* lease = get_dhcp_options(packet, OPT_LEASE_TIME); /* 51 */

		char ip[16];

		if (subnet != NULL) {
			if (!is_mask_allocated) {
				setnetmask(*subnet);
			}
			get_ip_str(ip, (uint8_t*)subnet);
			dprintf("DHCP: Received subnet mask: %s\n", ip);
			kfree_null(&subnet);
		}

		if (dns != NULL) {
			if (!is_dns_allocated) {
				setdnsaddr(*dns);
			}
			get_ip_str(ip, (uint8_t*)dns);
			dprintf("DHCP: Received DNS address: %s\n", ip);
			arp_prediscover((uint8_t*)dns);
			kfree_null(&dns);
		}

		if (gateway != NULL) {
			if (!is_gateway_allocated) {
				setgatewayaddr(*gateway);
			}
			get_ip_str(ip, (uint8_t*)gateway);
			dprintf("DHCP: Received gateway address: %s\n", ip);
			arp_prediscover((uint8_t*)gateway);
			kfree_null(&gateway);
		}

		/* Store server id and set timers */
		dhcp_server_id = *server_ip;
		kfree_null(&server_ip);

		if (lease != NULL) {
			uint32_t secs = ntohl(*lease);
			dprintf("DHCP: Lease expiry in %u seconds\n", secs);

			/* RFC defaults: T1 = 0.5 * lease, T2 = 0.875 * lease */
			int64_t lease_ms = (int64_t)secs * 1000;
			int64_t t1_ms = (int64_t)(secs / 2) * 1000;
			int64_t t2_ms = (int64_t)((secs * 7) / 8) * 1000;

			dhcp_expiry_ms = lease_ms;
			dhcp_t1_ms = t1_ms;
			dhcp_t2_ms = t2_ms;

			kfree_null(&lease);
		} else {
			/* No lease time provided: use conservative defaults (1 hour) */
			dhcp_expiry_ms = (int64_t)3600 * 1000;
			dhcp_t1_ms = (int64_t)1800 * 1000;
			dhcp_t2_ms = (int64_t)3150 * 1000;
		}

		dhcp_have_lease = true;
		dhcp_renew_sent = false;
		dhcp_rebind_sent = false;

		dhcp_selected_server = 0; /* exchange complete */

		kfree_null(&type);
		return;
	}

	if (*type == DHCPNAK) {
		/* Start over */
		dhcp_selected_server = 0;
		dhcp_have_lease = false;
		dhcp_renew_sent = false;
		dhcp_rebind_sent = false;

		dhcp_discover();

		kfree_null(&type);
		return;
	}

	/* Unhandled type */
	kfree_null(&type);
}

static void dhcp_background(void) {

	if (!dhcp_have_lease) {
		return;
	}

	/* Snapshot pre-decrement values to detect the 1 -> 0 edge exactly once */
	int64_t pre_exp = dhcp_expiry_ms;
	int64_t pre_t1  = dhcp_t1_ms;
	int64_t pre_t2  = dhcp_t2_ms;

	/* Decrement each counter once if active */
	if (dhcp_expiry_ms > 0) {
		dhcp_expiry_ms--;
	}
	if (dhcp_t1_ms > 0) {
		dhcp_t1_ms--;
	}
	if (dhcp_t2_ms > 0) {
		dhcp_t2_ms--;
	}

	/* Highest priority: lease expiry (fires exactly when pre_exp was 1) */
	if (pre_exp == 1) {
		dhcp_have_lease = false;
		dhcp_renew_sent = false;
		dhcp_rebind_sent = false;
		dhcp_selected_server = 0;
		dhcp_server_id = 0;
		dhcp_discover();
		return;
	}

	/* Next: T1 (only first time) */
	if (pre_t1 == 1) {
		if (!dhcp_renew_sent) {
			dhcp_renew_sent = true;
			dhcp_request_renew_unicast();
		}
		return;
	}

	/* Next: T2 (only first time) */
	if (pre_t2 == 1) {
		if (!dhcp_rebind_sent) {
			dhcp_rebind_sent = true;
			dhcp_request_rebind_broadcast();
		}
		return;
	}
}

void dhcp_discover() {
	uint8_t request_ip[4] = { 0, 0, 0, 0 };
	uint8_t dst_ip[4] = { 0xff, 0xff, 0xff, 0xff };
	dhcp_packet_t packet;

	memset(&packet, 0, sizeof(dhcp_packet_t));

	/* Roll a new transaction id for this discovery */
	static uint32_t xid_seed = 0x13579BDF;
	xid_seed += 0x01010101;
	dhcp_curr_xid = xid_seed;
	dhcp_selected_server = 0;

	uint16_t optsize = make_dhcp_packet(&packet, DHCPDISCOVER, request_ip, dhcp_curr_xid, 0);
	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, &packet, optsize);
}

void dhcp_request(uint8_t* request_ip, uint32_t xid, uint32_t server_ip) {
	uint8_t dst_ip[4] = { 0xff, 0xff, 0xff, 0xff };
	dhcp_packet_t packet;

	memset(&packet, 0, sizeof(dhcp_packet_t));

	uint16_t optsize = make_dhcp_packet(&packet, DHCPREQUEST, request_ip, xid, server_ip);
	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, &packet, optsize);
}

static uint16_t make_dhcp_request_ciaddr(dhcp_packet_t* packet, uint32_t xid, uint32_t ciaddr_netorder) {
	memset(packet, 0, sizeof(dhcp_packet_t));

	packet->op = DHCP_REQUEST;
	packet->hardware_type = HARDWARE_TYPE_ETHERNET;
	packet->hardware_addr_len = 6;
	packet->hops = 0;
	packet->xid = xid;

	/* Renew/Rebind: use ciaddr and do not set broadcast */
	packet->flags = 0;
	packet->client_ip = ciaddr_netorder;
	packet->server_ip = 0;
	packet->your_ip = 0;

	netdev_t* dev = get_active_network_device();
	if (dev == NULL) {
		return 0;
	}
	dev->get_mac_addr(packet->client_hardware_addr);

	uint8_t* options = packet->options;

	/* Magic cookie */
	*((uint32_t*)(options)) = htonl(0x63825363);
	options += 4;

	/* Message type = DHCPREQUEST */
	*(options++) = OPT_TYPE;
	*(options++) = 1;
	*(options++) = DHCPREQUEST;

	/* Client Identifier (Option 61): htype + MAC */
	*(options++) = OPT_CLIENT_MAC;               /* 61 */
	*(options++) = 7;                            /* length */
	*(options++) = HARDWARE_TYPE_ETHERNET;       /* 0x01 for Ethernet */
	dev->get_mac_addr(options);
	options += 6;

	/* No Requested-IP (50), no Server-ID (54) here */

	/* END */
	*(options++) = OPT_END;

	return (uint16_t)((size_t)options - (size_t)packet);
}

static void dhcp_request_renew_unicast(void) {
	/* T1: unicast to the original server with ciaddr = current IP */
	uint8_t dst_ip[4];
	uint8_t ci_ip[4];

	gethostaddr(ci_ip);  /* network order */
	memcpy(dst_ip, (uint8_t*)&dhcp_server_id, 4);

	dhcp_packet_t pkt;

	static uint32_t xid_seed = 0x2468ACE0;
	xid_seed += 0x01010101;
	dhcp_curr_xid = xid_seed;

	uint16_t optsize = make_dhcp_request_ciaddr(&pkt, dhcp_curr_xid, *((uint32_t*)ci_ip));
	if (optsize == 0) {
		return;
	}

	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, &pkt, optsize);
}

static void dhcp_request_rebind_broadcast(void) {
	/* T2: broadcast rebind with ciaddr = current IP */
	uint8_t dst_ip[4] = { 0xff, 0xff, 0xff, 0xff };
	uint8_t ci_ip[4];

	gethostaddr(ci_ip);  /* network order */

	dhcp_packet_t pkt;

	static uint32_t xid_seed = 0x97531BDF;
	xid_seed += 0x01010101;
	dhcp_curr_xid = xid_seed;

	uint16_t optsize = make_dhcp_request_ciaddr(&pkt, dhcp_curr_xid, *((uint32_t*)ci_ip));
	if (optsize == 0) {
		return;
	}

	udp_send_packet(dst_ip, DHCP_DST_PORT, DHCP_SRC_PORT, &pkt, optsize);
}

void* get_dhcp_options(dhcp_packet_t* packet, uint8_t type) {
	uint8_t* options = packet->options + 4;
	uint8_t curr_type = *options;

	while (curr_type != OPT_END && (uint64_t)options < (uint64_t)packet + sizeof(dhcp_packet_t)) {
		curr_type = *options;
		if (curr_type == OPT_END) {
			break;
		}

		uint8_t len = *(options + 1);

		if (curr_type == type) {
			void* ret = kmalloc(len);
			if (ret != NULL) {
				memcpy(ret, options + 2, len);
			}
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
	packet->flags = htons(0x8000); /* broadcast for discover/select */
	packet->client_ip = 0;
	packet->server_ip = server_ip;
	packet->your_ip = *((uint32_t*)request_ip);

	netdev_t* dev = get_active_network_device();
	if (dev == NULL) {
		return 0;
	}
	dev->get_mac_addr(packet->client_hardware_addr);

	uint8_t* options = packet->options;

	/* Magic cookie */
	*((uint32_t*)(options)) = htonl(0x63825363);
	options += 4;

	/* Message type */
	*(options++) = OPT_TYPE;
	*(options++) = 1;
	*(options++) = msg_type;

	/* Server identifier (if known) */
	if (server_ip != 0) {
		*(options++) = OPT_SERVER_IP;
		*(options++) = 4;
		memcpy(options, &server_ip, 4); /* already in network order */
		options += 4;
	}

	/* Client Identifier (Option 61): htype + MAC */
	*(options++) = OPT_CLIENT_MAC;               /* 61 */
	*(options++) = 7;                            /* length */
	*(options++) = HARDWARE_TYPE_ETHERNET;       /* 0x01 for Ethernet */
	dev->get_mac_addr(options);
	options += 6;

	/* Requested IP (Option 50) - only if we actually have one to ask for */
	if (*((uint32_t*)request_ip) != 0) {
		*(options++) = OPT_REQUESTED_IP; /* 50 */
		*(options++) = 4;
		memcpy(options, request_ip, 4);  /* network order expected */
		options += 4;
	}

	/* Host Name */
	const char* hostname = gethostname();
	uint8_t len = (uint8_t)strlen(hostname);
	*(options++) = OPT_HOSTNAME;
	*(options++) = len;
	memcpy(options, hostname, len);
	options += len;

	/* Parameter request list */
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

	/* END */
	*(options++) = OPT_END;

	return (uint16_t)((size_t)options - (size_t)packet);
}

void dhcp_init(void) {
	/* One-time: register the UDP handler for DHCP replies */
	udp_register_daemon(DHCP_DST_PORT, &dhcp_handle_packet, NULL);

	/* Background task is called every 1 ms in interrupt context */
	proc_register_idle(&dhcp_background, IDLE_BACKGROUND, 1);
}
