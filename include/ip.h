#pragma once

#include "kernel.h"

/**
 * @brief IP protocol types
 */
enum ip_type_t {
	IP_IPV4 = 4,
};

/**
 * @brief Fragmentation state
 */
enum ip_frag_t {
	IP_PACKET_NO_FRAGMENT = 2,
	IP_IS_LAST_FRAGMENT = 4,
};

/**
 * @brief Sub-protocols, TCP and UDP
 */
enum ip_proto_t {
	PROTOCOL_TCP = 6,
	PROTOCOL_UDP = 17,
};

/**
 * @brief Structure for an IP packet
 */
typedef struct ip_packet {
	char version_ihl_ptr[0];
	uint8_t version:4;
	uint8_t ihl:4;
	uint8_t tos;
	uint16_t length;
	uint16_t id;
	char flags_fragment_ptr[0];
	uint8_t flags:3;
	uint8_t fragment_offset_high:5;
	uint8_t fragment_offset_low;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t header_checksum;
	uint8_t src_ip[4];
	uint8_t dst_ip[4];
	uint8_t data[];
} __attribute__((packed)) ip_packet_t;

/**
 * @brief Convert network byte order IP to a string form for display
 * 
 * @param ip_str character buffer to fill, should be at least 13 bytes
 * @param ip raw 4 byte IP
 */
void get_ip_str(char* ip_str, uint8_t* ip);

/**
 * @brief Calculate checksum for IP packet
 * 
 * @param packet IP packet structure
 * @return uint16_t Checksum
 */
uint16_t ip_calculate_checksum(ip_packet_t* packet);

/**
 * @brief Send IP packet to the ethernet driver
 * 
 * @param dst_ip IP to send the packet to (255.255.255.255 for broadcast)
 * @param data raw IP packet to send
 * @param len data length
 */
void ip_send_packet(uint8_t* dst_ip, void* data, int len);

/**
 * @brief Handle incoming IP packet from the ethernet driver
 * 
 * @param packet IP packet
 */
void ip_handle_packet(ip_packet_t* packet);

/**
 * @brief Get the current IP address.
 * This could be allocated by DHCP or statically.
 * 
 * @param ip IP address buffer, should be at least 4 bytes
 * @return int nonzero if an IP address is available for use
 */
int gethostaddr(unsigned char* ip);

/**
 * @brief Set the current IP address.
 * 
 * @param ip 4 byte buffer for IP address
 */
void sethostaddr(const unsigned char* ip);

/**
 * @brief Set DNS address
 * This could be allocated by DHCP or statically.
 * 
 * @param dns DNS address
 */
void setdnsaddr(uint32_t dns);

/**
 * @brief Set gateway address
 * This could be allocated by DHCP or statically.
 * 
 * @param dns Gateway address
 */
void setgatewayaddr(uint32_t gateway);

/**
 * @brief Get DNS address
 * This could be allocated by DHCP or statically.
 * 
 * @return uint32_t DNS address
 */
uint32_t getdnsaddr();

/**
 * @brief Get gateway address
 * This could be allocated by DHCP or statically.
 * 
 * @return uint32_t gateway address
 */
uint32_t getgatewayaddr();