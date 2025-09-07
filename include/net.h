/**
 * @file net.h
 * @author Craig Edwards
 * @brief Networking device and protocol definitions.
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#include "kernel.h"

/**
 * @brief Represents an address bound to a protocol on an interface.
 *
 * Designed to be flexible for different protocols (e.g. IPv4, IPv6, IPX,
 * Econet). The @p length specifies the number of bytes in the address,
 * and @p bytes holds the address in host byte order.
 *
 * Example: for IPv4, @p length is 4 and @p bytes points to a `uint32_t`.
 * The @p name is a human‑readable label such as "ip.address" or "ip.gateway".
 */
typedef struct net_address {
	char name[16];     /**< Human‑readable label for the address. */
	uint8_t length;    /**< Length of the address in bytes. */
	uint8_t* bytes;    /**< Address bytes (host byte order). */
} net_address_t;

/**
 * @brief Represents a protocol, or group of related protocols, that can be
 * attached to an interface and have addresses.
 *
 * Currently only Ethernet devices are supported, covering the majority of
 * consumer hardware. Protocols may have dependencies (e.g. ARP required by IPv4),
 * which are added automatically when needed.
 */
typedef struct netproto {
	char name[16];             /**< Human‑readable name (e.g. "ip", "ipx"). */
	uint16_t ethernet_protocol_id; /**< Ethernet protocol ID. */
	uint16_t num_addresses;    /**< Number of bound addresses. */
	net_address_t* addresses;  /**< Array of bound addresses. */
} netproto_t;

/**
 * @brief Flags for the state of a network device.
 */
enum netdev_flags_t {
	ADMINISTRATIVELY_DOWN = 1, /**< Device marked down by the user. */
	CONNECTED              = 2 /**< Device has carrier and is connected. */
};

typedef struct {
	char *hostname;   /* strdup’d, may be NULL if not set */
	uint32_t ip;      /* 0 = dhcp/unset */
	uint32_t netmask; /* 0 = dhcp/unset */
	uint32_t gateway; /* 0 = dhcp/unset */
	uint32_t dns;     /* 0 = dhcp/unset */
} net_config;

bool parse_network_config(const unsigned char *buf, size_t len, net_config *out);

/** @brief Callback to retrieve MAC address. */
typedef void (*net_get_mac)(uint8_t*);

/** @brief Callback to send a packet through the device. */
typedef bool (*net_send_packet)(void* data, uint16_t len);

/**
 * @brief Represents a physical Ethernet network device.
 *
 * Each device may have zero or more attached protocols. Each protocol
 * intercepts traffic by its Ethernet ID. Some protocols depend on others,
 * which are attached automatically.
 */
typedef struct netdev {
	uint32_t deviceid;          /**< Unique device identifier. */
	char name[16];              /**< Device name (e.g. "rtl0"). */
	char* description;          /**< Human‑readable description of the device. */
	void* opaque;               /**< Driver‑specific storage pointer. */
	uint16_t speed;             /**< Speed in megabytes per second. */
	uint8_t flags;              /**< Flags from ::netdev_flags_t. */
	uint16_t mtu;               /**< Maximum transmission unit. */
	uint8_t num_netprotos;      /**< Number of attached protocols. */
	netproto_t* netproto;       /**< Array of attached protocols. */
	net_get_mac get_mac_addr;   /**< Retrieve MAC address. */
	net_send_packet send_packet;/**< Send a packet via the interface. */
	struct netdev* next;        /**< Next device in the linked list. */
} netdev_t;

/* ------------------------------------------------------------------------- */
/* Byte order conversion                                                      */
/* ------------------------------------------------------------------------- */

/**
 * @brief Host‑to‑network conversion for sub‑byte fields.
 * @param byte Input byte.
 * @param num_bits Number of bits in the high nibble.
 * @return Byte in network order.
 */
uint8_t htonb(uint8_t byte, int num_bits);

/**
 * @brief Network‑to‑host conversion for sub‑byte fields.
 * @param byte Input byte.
 * @param num_bits Number of bits in the high nibble.
 * @return Byte in host order.
 */
uint8_t ntohb(uint8_t byte, int num_bits);

/**
 * @brief Convert 16‑bit value from host to network byte order.
 */
uint16_t htons(uint16_t hostshort);

/**
 * @brief Convert 32‑bit value from host to network byte order.
 */
uint32_t htonl(uint32_t hostlong);

/**
 * @brief Convert 16‑bit value from network to host byte order.
 */
uint16_t ntohs(uint16_t netshort);

/**
 * @brief Convert 32‑bit value from network to host byte order.
 */
uint32_t ntohl(uint32_t netlong);

/* ------------------------------------------------------------------------- */
/* Network stack control                                                      */
/* ------------------------------------------------------------------------- */

/**
 * @brief Bring the network stack online.
 *
 * Initialises ARP, IP, TCP, DHCP, and DNS subsystems.
 */
void network_up(void);

/**
 * @brief Shut the network stack down.
 */
void network_down(void);

/* ------------------------------------------------------------------------- */
/* Device management                                                          */
/* ------------------------------------------------------------------------- */

/**
 * @brief Register a new network device.
 * @param newdev Pointer to the device structure.
 * @return true on success.
 */
bool register_network_device(netdev_t* newdev);

/**
 * @brief Find a network device by name.
 * @param name Device name.
 * @return Pointer to the device, or NULL if not found.
 */
netdev_t* find_network_device(const char* name);

/**
 * @brief Get the most recently registered network device.
 * @return Pointer to the active device, or NULL if none exist.
 */
netdev_t* get_active_network_device(void);

/**
 * @brief Configure the active network interface from /system/config/network.conf
 *
 * This function is called by a network driver once it has initialised successfully.
 * It reads the global configuration file and applies hostname, IP address, netmask,
 * gateway, and DNS settings. Each field may be specified either as a literal value
 * or as the string "dhcp" to request assignment from a DHCP server.
 *
 * Behaviour:
 * - If the host already has an address (gethostaddr() returns true), the function
 *   exits early without reconfiguring.
 * - If /system/config/network.conf does not exist or cannot be read, DHCP discovery
 *   is attempted for all parameters.
 * - For each field set to a literal in the config, the corresponding setter is
 *   invoked immediately (sethostaddr, setnetmask, setgatewayaddr, setdnsaddr).
 * - If any field is marked "dhcp" (represented internally as 0), a DHCP request
 *   is started. DHCP will fill only the unset fields and will not overwrite static
 *   values already applied.
 * - If a static IP address is configured, a gratuitous ARP announcement is broadcast
 *   to advertise the address on the local network.
 *
 * The hostname defaults to "retrorocket" if not present in the configuration.
 *
 * @note Currently only a single active NIC plus loopback are supported.
 * @note The caller must ensure a network device is present before invoking this.
 */
void network_setup();