#pragma once

#include "kernel.h"

/**
 * @brief Raw structure for UDP
 */
typedef struct udp_packet {
	uint16_t src_port; // source port
	uint16_t dst_port; // destination port
	uint16_t length; // packet length
	uint16_t checksum; // optional checksum
	uint8_t data[]; // raw packet content
} __attribute__((packed)) udp_packet_t;

/**
 * @brief This returns 0 for all checksums as it is optional in ipv4
 * 
 * @param packet packet data
 * @return uint16_t checksum (always 0)
 */
uint16_t udp_calculate_checksum(udp_packet_t* packet);

/**
 * @brief Send a UDP packet via the ethernet driver
 * 
 * @param dst_ip destination IP address
 * @param src_port source port number
 * @param dst_port destination port number
 * @param data raw packet data
 * @param len raw packet length
 */
void udp_send_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, int len);

/**
 * @brief Handle UDP packet from the IP driver
 * 
 * @param packet raw UDP packet
 * @param length UDP packet length
 */
void udp_handle_packet(udp_packet_t* packet, size_t length);
