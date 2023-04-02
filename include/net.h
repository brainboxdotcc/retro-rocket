#pragma once

#include "kernel.h"

/**
 * @brief Represents an address that is bound to a protocol on an interface.
 * This is kept as flexible as possible, to represent potentially different
 * protocol's forms of addresses, e.g. IPv4, IPv6, IPX, Econet etc. The length
 * indicates the number of bytes in the address, and the bytes field contains
 * the actual address in host byte order. For example in IPv4, length would
 * be 4 and bytes would essentially point at a uint32_t. The name is purely
 * a human readable label for use in commands. The label might be 'ip.address',
 * 'ip.gateway' etc.
 */
typedef struct net_address {
	char name[16]; // name of net address, e.g. 'ip.address', 'econet.broadcast' etc
	uint8_t length; // address length
	uint8_t* bytes; // address bytes
} net_address_t;

/**
 * @brief Represents a protocol, or group of related protocols, that can be attached
 * to an interface and can have addresses. At present only ethernet devices are
 * supported, this covers 99% of the scenarios we might see in the wild on consumer
 * machines.
 */
typedef struct netproto {
	char name[16]; // human readable name, e.g. 'ip', 'ipx', 'econet'
	uint16_t ethernet_protocol_id; // ethernet id for identifying the protocol
	uint16_t num_addresses; // number of bound and related addresses
	net_address_t* addresses; // array of bound and related addresses
} netproto_t;

/**
 * @brief Potential flags for the state of a network device
 */
enum netdev_flags_t {
	ADMINISTRATIVELY_DOWN = 1, // Device marked as down by the user
	CONNECTED = 2, // Device connected (has carrier)
};

/**
 * @brief An ethernet network device, this relates directly to a physical
 * network card in the machine. A device may have zero or more attached
 * protocols, each protocol intercepts an ethernet id number. Where a
 * protocol requires another protocol to be of use, e.g. ARP with IPv4,
 * the dependency will be added automatically by other protocols that
 * require it.
 */
typedef struct netdev {
	char name[16]; // a device name such as rtl0
	char* description; // Human readable description of the device type
	uint16_t speed; // Speed in megabytes per second
	uint8_t flags; // Flags from netdev_flags_t
	uint16_t mtu; // MTU of the device (this may filter down to protocols, or protocols may set their own)
	uint8_t num_netprotos; // number of attached network protocols
	netproto_t* netproto; // array of network protocols
} netdev_t;

/**
 * @brief Host to network byte
 * 
 * @param byte input
 * @param num_bits number of bits of high nibble
 * @return uint8_t network byte order
 */
uint8_t htonb(uint8_t byte, int num_bits);

/**
 * @brief Network to host byte
 * 
 * @param byte input
 * @param num_bits number of bits of high nibble
 * @return uint8_t host byte order
 */
uint8_t ntohb(uint8_t byte, int num_bits);

/**
 * @brief Host to network short
 * 
 * @param hostshort input
 * @return uint16_t network order
 */
uint16_t htons(uint16_t hostshort);

/**
 * @brief Host to network long
 * 
 * @param hostlong input
 * @return uint32_t network order
 */
uint32_t htonl(uint32_t hostlong);

/**
 * @brief Network to host short
 * 
 * @param netshort input
 * @return uint16_t host order
 */
uint16_t ntohs(uint16_t netshort);

/**
 * @brief Network to host long
 * 
 * @param netlong input
 * @return uint32_t host order
 */
uint32_t ntohl(uint32_t netlong);

void network_up();

void network_down();