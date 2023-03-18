#pragma once

#include "kernel.h"
#include <stdint.h>

/**
 * @brief Low level ethernet packet types
 */
enum ethernet_type {
	ETHERNET_TYPE_ARP = 0x0806,
	ETHERNET_TYPE_IP  = 0x0800,
};

/**
 * @brief Ethernet hardware type
 */
#define HARDWARE_TYPE_ETHERNET 0x01

/**
 * @brief There is actually more to an ethernet frame than this
 * (see the STD documents!) but we don't have access to it from the
 * software.
 */
typedef struct ethernet_frame {
	uint8_t dst_mac_addr[6]; // Destination MAC address
	uint8_t src_mac_addr[6]; // Source MAC address
	uint16_t type; // Packet type
	uint8_t data[]; // Raw data
} __attribute__((packed)) ethernet_frame_t;

/**
 * @brief Send a raw ethernet packet to the network card driver
 * 
 * @param dst_mac_addr destination MAC address (FF:FF:FF:FF:FF:FF for broadcast)
 * @param data raw packet data
 * @param len packet data length
 * @param protocol protocol type
 * @return int nonzero on successful queue of packet
 */
int ethernet_send_packet(uint8_t* dst_mac_addr, uint8_t* data, int len, uint16_t protocol);

/**
 * @brief Handle inbound packet via interrupt from network card driver
 * 
 * @param packet raw packet data
 * @param len packet data length
 */
void ethernet_handle_packet(ethernet_frame_t * packet, int len);

