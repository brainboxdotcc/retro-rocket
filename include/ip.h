/**
 * @file ip.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

#define ETHERNET_TYPE_IP 0x0800
#define ETHERNET_TYPE_IP6 0x86DD

/**
 * @brief IP protocol types
 */
enum ip_type_t {
	IP_IPV4 = 4,
};

/**
 * @brief Sub-protocols, TCP and UDP
 */
enum ip_proto_t {
	PROTOCOL_ICMP   = 1,   /* Internet Control Message Protocol */
	PROTOCOL_IGMP   = 2,   /* Internet Group Management Protocol */
	PROTOCOL_TCP    = 6,   /* Transmission Control Protocol */
	PROTOCOL_UDP    = 17,  /* User Datagram Protocol */
	PROTOCOL_GRE    = 47,  /* Generic Routing Encapsulation */
	PROTOCOL_ESP    = 50,  /* Encapsulating Security Payload (IPsec) */
	PROTOCOL_AH     = 51,  /* Authentication Header (IPsec) */
	PROTOCOL_IPV6   = 41,  /* IPv6 encapsulation */
	PROTOCOL_OSPF   = 89,  /* Open Shortest Path First */
	PROTOCOL_PIM    = 103, /* Protocol Independent Multicast */
	PROTOCOL_SCTP   = 132, /* Stream Control Transmission Protocol */
	PROTOCOL_VRRP   = 112, /* Virtual Router Redundancy Protocol */
};

/**
 * @brief Structure and bitfields for representing IP fragmentation.
 * Used within an ip_packet_t
 */
typedef union ip_frag {
    uint16_t bits;
    struct {
	uint8_t reserved_zero		: 1; // always 0
	uint8_t dont_fragment		: 1; // if 1 we must never fragment this packet
	uint8_t more_fragments_follow	: 1; // if 1, more fragments follow this one
	uint8_t fragment_offset_high	: 5; // high bits of fragment offset
	uint8_t fragment_offset_low	: 8; // low bits of fragment offset
    };
} __attribute__((packed)) ip_frag_t;

typedef union tos_flags {
	uint8_t bits;
	struct {
		uint8_t dscp:6; // Differentiated services code point
		uint8_t ecn:2; // Explicit congestion notification
	};
} __attribute__((packed)) tos_flags_t;

/**
 * @brief Structure for an IP packet
 */
typedef struct ip_packet {
	char version_ihl_ptr[0]; // pointer to IHL for bit twiddling
	uint8_t version:4; // version
	uint8_t ihl:4; // ip header length in 4 byte words
	tos_flags_t tos; // type-of-service (TOS)
	uint16_t length; // length of ip_packet_t + data
	uint16_t id; // incrementing id
	char flags_fragment_ptr[0]; // pointer to fragment info for bit twiddling
	ip_frag_t frag; // fragmentation info
	uint8_t ttl; // Packet TTL
	uint8_t protocol; // Inner protocol contained in IP packet, e.g. TCP, UDP, ICMP
	uint16_t header_checksum; // checksum
	uint8_t src_ip[4]; // source IP
	uint8_t dst_ip[4]; // destination IP
	uint8_t options[0]; // optional, included if ihl > 5, overlaps data!
	uint8_t data[]; // Packet data
} __attribute__((packed)) ip_packet_t;

typedef struct ip_packet_frag {
	uint32_t offset;
	ip_packet_t* packet;
	struct ip_packet_frag* next;
	struct ip_packet_frag* prev;
} ip_packet_frag_t;

typedef struct ip_fragmented_packet_parts {
	uint16_t id;
	uint32_t size;
	ip_packet_frag_t* ordered_list;
	uint64_t last_seen_ticks;
} ip_fragmented_packet_parts_t;

typedef struct packet_queue_item {
	ip_packet_t* packet;
	uint8_t arp_tries;
	time_t last_arp;
	struct packet_queue_item* next;
} packet_queue_item_t;

/**
 * @brief Convert network byte order IP to a string form for display
 * 
 * @param ip_str character buffer to fill, should be at least 13 bytes
 * @param ip raw 4 byte IP
 */
void get_ip_str(char* ip_str, const uint8_t* ip);

/**
 * @brief Convert a string IP to host byte order IP address
 * 
 * @param ip_str ip as string e.g. "127.0.0.1"
 * @return uint32_t IP in host byte order
 */
uint32_t str_to_ip(const char* ip_str);

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
 * @param protocol Sub-protocol encapsulated inside the packet, e.g. UDP, TCP
 */
void ip_send_packet(uint8_t* dst_ip, void* data, uint16_t len, uint8_t protocol);

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

/**
 * @brief Set network mask
 * 
 * @param nm network mask
 */
void setnetmask(uint32_t nm);

/**
 * @brief Get network mask
 * 
 * @return uint32_t network mask
 */
uint32_t getnetmask();

void sethostname(const char* hostname);

const char* gethostname();

/**
 * @brief Initialise IP protocol
 */
void ip_init();

/**
 * @brief Called from the local APIC timer interrupt 50 times a second.
 * Attempts to resolve ARP requests for queued packets that we don't have
 * ARP for yet, and retries ARP requests up to 3 times before dropping
 * packets we cannot route.
 */
void ip_idle();