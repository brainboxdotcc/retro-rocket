#include <kernel.h>

netdev_t* networkdevices = NULL;

#if __BYTE_ORDER == __LITTLE_ENDIAN

uint16_t flip_short(uint16_t short_int) {
	uint32_t first_byte = *((uint8_t*)(&short_int));
	uint32_t second_byte = *((uint8_t*)(&short_int) + 1);
	return (first_byte << 8) | (second_byte);
}

uint32_t flip_long(uint32_t long_int) {
	uint32_t first_byte = *((uint8_t*)(&long_int));
	uint32_t second_byte = *((uint8_t*)(&long_int) + 1);
	uint32_t third_byte = *((uint8_t*)(&long_int)  + 2);
	uint32_t fourth_byte = *((uint8_t*)(&long_int) + 3);
	return (first_byte << 24) | (second_byte << 16) | (third_byte << 8) | (fourth_byte);
}

uint8_t flip_byte(uint8_t byte, int num_bits) {
	uint8_t t = byte << (8 - num_bits);
	return t | (byte >> num_bits);
}

uint8_t htonb(uint8_t byte, int num_bits) {
	return flip_byte(byte, num_bits);
}

uint8_t ntohb(uint8_t byte, int num_bits) {
	return flip_byte(byte, 8 - num_bits);
}

uint16_t htons(uint16_t hostshort) {
	return flip_short(hostshort);
}

uint32_t htonl(uint32_t hostlong) {
	return flip_long(hostlong);
}

uint16_t ntohs(uint16_t netshort) {
	return flip_short(netshort);
}

uint32_t ntohl(uint32_t netlong) {
	return flip_long(netlong);
}

#else

/* Big endian machines like motorola, DEC, MIPS, Sparc are Big-Endian.
 * Big-endian architectures match the binary layout of network byte order,
 * so on those systems the faff above isnt required. Yay for them!
 */

uint16_t flip_short(uint16_t short_int) {
	return short_int;
}

uint32_t flip_long(uint32_t long_int) {
	return long_int;
}

uint8_t flip_byte(uint8_t byte, int num_bits) {
	return byte;
}

uint8_t htonb(uint8_t byte, int num_bits) {
	return byte;
}

uint8_t ntohb(uint8_t byte, int num_bits) {
	return byte;
}

uint16_t htons(uint16_t hostshort) {
	return hostshort;
}

uint32_t htonl(uint32_t hostlong) {
	return hostlong;
}

uint16_t ntohs(uint16_t netshort) {
	return netshort;
}

uint32_t ntohl(uint32_t netlong) {
	return netlong;
}

#endif

void network_up()
{
	arp_init();
	ip_init();
	tcp_init();
	dhcp_init();
	init_dns();
	if (tls_global_init() == -1) {
		dprintf("TLS initialisation error: TLS will be unavailable!\n");
	}
}

void network_down()
{
}

bool register_network_device(netdev_t* newdev) {
	/* Add the new network device to the start of the list */
	dprintf("Registered network device '%s'\n", newdev->name);
	newdev->next = networkdevices;
	networkdevices = newdev;
	return true;
}

netdev_t* get_active_network_device() {
	/* First in list, most recent to be detected */
	return networkdevices;
}

netdev_t* find_network_device(const char* name) {
	netdev_t* cur = networkdevices;
	for(; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

void network_setup() {
	uint8_t addr[4];
	if (gethostaddr(addr)) {
		return;
	}
	dprintf("Network setup...\n");
	fs_directory_entry_t* info = fs_get_file_info("/system/config/network.conf");
	if (!info || (info->flags & FS_DIRECTORY) != 0) {
		/* No config file */
		dhcp_discover();
		return;
	}
	net_config config = {};
	unsigned char buffer[info->size];
	if (fs_read_file(info, 0, info->size, buffer)) {
		parse_network_config(buffer, info->size, &config);
		/* If any of these contain 0.0.0.0 ->(uint32_t)0, these functions are no-op */
		if (!config.hostname || *config.hostname == 0) {
			config.hostname = strdup("retrorocket");
		}
		sethostname(config.hostname);
		sethostaddr((const unsigned char*)&config.ip);
		setdnsaddr(config.dns);
		setnetmask(config.netmask);
		setgatewayaddr(config.gateway);
		dprintf(
			"Config: hostname=%s ip=%08x dns=%08x netmask=%08x gateway=%08x\n",
			config.hostname,
			config.ip, config.dns, config.netmask, config.gateway
		);
		/* If anything is non-static, request dhcp */
		if (config.ip == 0 || config.dns == 0 || config.gateway == 0 || config.netmask == 0) {
			/* A dhcp assignment won't overwrite statically defined values set above */
			dprintf("One or more configuration elements are dhcp, requesting\n");
			dhcp_discover();
		}
		if (config.ip) {
			arp_announce_my_ip();
		}
		if (config.dns) {
			arp_prediscover((uint8_t*)&config.dns);
		}
		if (config.gateway) {
			arp_prediscover((uint8_t*)&config.gateway);
		}
		kfree_null(&config.hostname);
	} else {
		dprintf("No network.conf, dhcp setup\n");
		/* Unable to read config file */
		dhcp_discover();
	}
}

bool parse_network_config(const unsigned char *buf, size_t len, net_config *out) {
	if (!buf || !out) {
		return false;
	}

	memset(out, 0, sizeof(*out));

	const char *p = (const char*)buf;
	const char *end = (const char*)buf + len;
	char line[len];

	while (p < end) {
		/* extract one line */
		size_t l = 0;
		while (p < end && *p != '\n' && l + 1 < sizeof(line)) {
			line[l++] = *p++;
		}
		if (p < end && *p == '\n') {
			p++;
		}
		line[l] = '\0';

		/* skip comments/blank */
		char *s = line;
		while (*s == ' ' || *s == '\t') {
			s++;
		}
		if (*s == '#' || *s == '\0') {
			continue;
		}

		/* split key/value */
		char *key = s;
		while (*s && *s != ' ' && *s != '\t') {
			s++;
		}
		if (*s == '\0') {
			continue;
		}
		*s++ = '\0';
		while (*s == ' ' || *s == '\t') {
			s++;
		}
		if (*s == '\0') {
			continue;
		}
		char *val = s;

		/* dispatch */
		if (strcmp(key, "hostname") == 0) {
			kfree_null(&out->hostname);
			out->hostname = strdup(val);
		} else if (strcmp(key, "ip") == 0) {
			out->ip = (strcmp(val, "dhcp") == 0) ? 0 : str_to_ip(val);
		} else if (strcmp(key, "netmask") == 0) {
			out->netmask = (strcmp(val, "dhcp") == 0) ? 0 : str_to_ip(val);
		} else if (strcmp(key, "gateway") == 0) {
			out->gateway = (strcmp(val, "dhcp") == 0) ? 0 : str_to_ip(val);
		} else if (strcmp(key, "dns") == 0) {
			/* honour only the first DNS for now */
			char *comma = strchr(val, ',');
			if (comma) {
				*comma = '\0';
			}
			out->dns = (strcmp(val, "dhcp") == 0) ? 0 : str_to_ip(val);
		}
	}

	return true;
}
