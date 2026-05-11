/**
 * @file icmp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

#include "kernel.h"

/**
 * @brief ICMP packet types
 */
enum icmp_type_t {
	/** Echo reply */
	ICMP_ECHO_REPLY = 0,

	/** Destination unreachable */
	ICMP_DESTINATION_UNREACHABLE = 3,

	/** Source quench, deprecated */
	ICMP_SOURCE_QUENCH = 4,

	/** Redirect */
	ICMP_REDIRECT = 5,

	/** Echo request */
	ICMP_ECHO = 8,

	/** Time exceeded */
	ICMP_TIME_EXCEEDED = 11,

	/** Parameter problem */
	ICMP_PARAMETER_PROBLEM = 12,

	/** Timestamp request */
	ICMP_TIMESTAMP = 13,

	/** Timestamp reply */
	ICMP_TIMESTAMP_REPLY = 14,

	/** Information request */
	ICMP_INFORMATION_REQUEST = 15,

	/** Information reply */
	ICMP_INFORMATION_REPLY = 16,
};

/**
 * @brief ICMP destination unreachable reason codes
 */
enum icmp_unreachable_code_t {
	/** Destination network is unreachable */
	ICMP_NET_UNREACHABLE = 0,

	/** Destination host is unreachable */
	ICMP_HOST_UNREACHABLE = 1,

	/** Destination protocol is unreachable */
	ICMP_PROTOCOL_UNREACHABLE = 2,

	/** Destination port is unreachable */
	ICMP_PORT_UNREACHABLE = 3,

	/** Fragmentation is required but the IPv4 DF bit was set */
	ICMP_FRAGMENTATION_NEEDED = 4,

	/** Source route failed */
	ICMP_SOURCE_ROUTE_FAILED = 5,
};

/**
 * @brief ICMP time exceeded reason codes
 */
enum icmp_time_exceeded_code_t {
	/** Packet TTL expired in transit */
	ICMP_TTL_EXCEEDED = 0,

	/** Fragment reassembly timeout expired */
	ICMP_FRAGMENT_REASSEMBLY_TIME_EXCEEDED = 1,
};

/**
 * @brief ICMP redirect reason codes
 */
enum icmp_redirect_code_t {
	/** Redirect traffic for a network */
	ICMP_REDIRECT_NETWORK = 0,

	/** Redirect traffic for a host */
	ICMP_REDIRECT_HOST = 1,

	/** Redirect traffic for a TOS/network combination */
	ICMP_REDIRECT_TOS_NETWORK = 2,

	/** Redirect traffic for a TOS/host combination */
	ICMP_REDIRECT_TOS_HOST = 3,
};

/**
 * @brief Generic ICMP packet carrying a quoted IPv4 datagram
 *
 * Used for destination unreachable and time exceeded messages. For
 * fragmentation-needed messages, unused carries the next-hop MTU in network
 * byte order.
 */
typedef struct icmp_packet {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP type-specific code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Type-specific auxiliary field */
	uint32_t unused;

	/** Quoted IPv4 datagram that triggered the ICMP message */
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_packet_t;

/**
 * @brief ICMP redirect packet
 */
typedef struct icmp_redirect_packet {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP redirect reason code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Replacement gateway IPv4 address */
	uint32_t gateway;

	/** Quoted IPv4 datagram that triggered the redirect */
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_redirect_packet_t;

/**
 * @brief ICMP information request/reply packet
 */
typedef struct icmp_information {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP type-specific code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Request identifier */
	uint16_t id;

	/** Request sequence number */
	uint16_t seq;
} __attribute__((packed)) icmp_information_t;

/**
 * @brief ICMP parameter problem packet
 */
typedef struct icmp_parameter_problem_packet {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP parameter problem code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Offset of invalid byte within the IPv4 header */
	uint8_t problem_ptr;

	/** Reserved field */
	uint16_t unused1;

	/** Reserved field */
	uint16_t unused2;

	/** Quoted IPv4 datagram containing the invalid header */
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_parameter_problem_packet_t;

/**
 * @brief ICMP echo request/reply packet
 */
typedef struct icmp_echo_packet {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP type-specific code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Echo identifier */
	uint16_t id;

	/** Echo sequence number */
	uint16_t seq;
} __attribute__((packed)) icmp_echo_packet_t;

/**
 * @brief ICMP timestamp request/reply packet
 */
typedef struct icmp_timestamp_packet {
	/** ICMP packet type */
	uint8_t type;

	/** ICMP type-specific code */
	uint8_t code;

	/** Internet checksum of the ICMP packet */
	uint16_t checksum;

	/** Request identifier */
	uint16_t id;

	/** Request sequence number */
	uint16_t seq;

	/** Sender originate timestamp */
	uint32_t originate_timestamp;

	/** Receiver receive timestamp */
	uint32_t receive_timestamp;

	/** Sender transmit timestamp */
	uint32_t transmit_timestamp;
} __attribute__((packed)) icmp_timestamp_packet_t;

/**
 * @brief Callback for protocol handlers interested in ICMP unreachable messages
 *
 * The quoted IPv4 datagram identifies the protocol endpoint that triggered the
 * ICMP error. The MTU value is only meaningful for ICMP_FRAGMENTATION_NEEDED.
 *
 * @param quoted_ip Quoted IPv4 datagram from the ICMP payload
 * @param code ICMP unreachable reason code
 * @param mtu Path MTU for ICMP_FRAGMENTATION_NEEDED, otherwise zero
 */
typedef void (*icmp_unreachable_handler_t)(ip_packet_t*, uint8_t, uint16_t);

/**
 * @brief Register a handler for ICMP unreachable notifications
 *
 * @param protocol IP protocol number to register for
 * @param handler Callback function
 */
void icmp_register_unreachable_handler(uint8_t protocol, icmp_unreachable_handler_t handler);

/**
 * @brief Send an ICMP echo request
 *
 * @param destination Destination IPv4 address
 * @param id Echo identifier
 * @param seq Echo sequence number
 */
void icmp_send_echo(uint8_t* destination, uint16_t id, uint16_t seq);

/**
 * @brief Send an ICMP time exceeded packet
 *
 * @param original_ip Original IPv4 packet that triggered the error
 * @param code ICMP time exceeded reason code
 */
void icmp_send_time_exceeded(ip_packet_t* original_ip, uint8_t code);

/**
 * @brief Send an ICMP destination unreachable packet
 *
 * @param original_ip Original IPv4 packet that triggered the error
 * @param code ICMP unreachable reason code
 */
void icmp_send_destination_unreachable(ip_packet_t* original_ip, uint8_t code);

/**
 * @brief Send an ICMP parameter problem packet
 *
 * @param original_ip Original IPv4 packet that triggered the error
 * @param pointer_offset Offset of invalid byte within the IPv4 header
 */
void icmp_send_parameter_problem(ip_packet_t* original_ip, uint8_t pointer_offset);

/**
 * @brief Send an ICMP redirect packet
 *
 * @param original_ip Original IPv4 packet that triggered the redirect
 * @param code ICMP redirect reason code
 * @param new_gateway_ip Replacement gateway IPv4 address
 */
void icmp_send_redirect(ip_packet_t* original_ip, uint8_t code, uint32_t new_gateway_ip);

/**
 * @brief Send an ICMP fragmentation needed packet
 *
 * @param original_ip Original IPv4 packet that could not be forwarded
 * @param mtu Path MTU required for successful forwarding
 */
void icmp_send_fragmentation_needed(ip_packet_t* original_ip, uint16_t mtu);

/**
 * @brief Initialise the ICMP subsystem
 */
void icmp_init();