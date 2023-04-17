/**
 * @file udp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#pragma once

#include "kernel.h"

// Interrupt handler definition
typedef void (*udp_daemon_handler)(uint16_t, void*, uint32_t);

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
void udp_send_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, uint16_t len);

/**
 * @brief Handle UDP packet from the IP driver
 * 
 * @param ip Encapsulating IP packet
 * @param packet raw UDP packet
 * @param length UDP packet length
 */
void udp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, udp_packet_t* packet, size_t len);

/**
 * @brief Register a daemon function to listen on a udp dest port
 * 
 * @param dst_port destination port to listen on. If 0 is passed, a random port above or equal to 1024
 * is allocated for use and will be returned as the return value.
 * @param handler handler for incoming packets
 * @return port number that was allocated
 */
uint16_t udp_register_daemon(uint16_t dst_port, udp_daemon_handler handler);