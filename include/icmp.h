/**
 * @file icmp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

enum icmp_type_t {
	ICMP_ECHO_REPLY = 0,
	ICMP_DESTINATION_UNREACHABLE = 3,
	ICMP_SOURCE_QUENCH = 4,
	ICMP_REDIRECT = 5,
	ICMP_ECHO = 8,
	ICMP_TIME_EXCEEDED = 11,
	ICMP_PARAMETER_PROBLEM = 12,
	ICMP_TIMESTAMP = 13,
	ICMP_TIMESTAMP_REPLY = 14,
	ICMP_INFORMATION_REQUEST = 15,
	ICMP_INFORMATION_REPLY = 16,
};

enum icmp_unreachable_code_t {
	ICMP_NET_UNREACHABLE = 0,
	ICMP_HOST_UNREACHABLE = 1,
	ICMP_PROTOCOL_UNREACHABLE = 2,
	ICMP_PORT_UNREACHABLE = 3,
	ICMP_FRAGMENTATION_NEEDED = 4,
	ICMP_SOURCE_ROUTE_FAILED = 5,
};

enum icmp_time_exceeded_code_t {
	ICMP_TTL_EXCEEDED = 0,
	ICMP_FRAGMENT_REASSEMBLY_TIME_EXCEEDED = 1,
};

enum icmp_redirect_code_t {
	ICMP_REDIRECT_NETWORK = 0,
	ICMP_REDIRECT_HOST = 1,
	ICMP_REDIRECT_TOS_NETWORK = 2,
	ICMP_REDIRECT_TOS_HOST = 3,
};

typedef struct icmp_packet {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t unused;
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_packet_t;

typedef struct icmp_redirect_packet {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t gateway;
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_redirect_packet_t;

typedef struct icmp_information {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t seq;
} __attribute__((packed)) icmp_information_t;

typedef struct icmp_parameter_problem_packet {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t problem_ptr;
	uint16_t unused1;
	uint16_t unused2;
	ip_packet_t original_datagram;
} __attribute__((packed)) icmp_parameter_problem_packet_t;

typedef struct icmp_echo_packet {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t seq;
} __attribute__((packed)) icmp_echo_packet_t;

typedef struct icmp_timestamp_packet {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t seq;
	uint32_t originate_timestamp;
	uint32_t receive_timestamp;
	uint32_t transmit_timestamp;
} __attribute__((packed)) icmp_timestamp_packet_t;

/**
 * @brief Handle ICMP packet from the IP driver
 * 
 * @param ip Encapsulating IP packet
 * @param packet raw ICMP packet
 * @param length ICMP packet length
 */
void icmp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, icmp_packet_t* packet, size_t len);

void icmp_send_echo(uint8_t* destination, uint16_t id, uint16_t seq);

void icmp_send_time_exceeded(ip_packet_t* original_ip, uint8_t code);

void icmp_send_destination_unreachable(ip_packet_t* original_ip, uint8_t code);

void icmp_send_parameter_problem(ip_packet_t* original_ip, uint8_t pointer_offset);

void icmp_send_redirect(ip_packet_t* original_ip, uint8_t code, uint32_t new_gateway_ip);

void icmp_send_fragmentation_needed(ip_packet_t* original_ip, uint16_t mtu);
