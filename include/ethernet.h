/**
 * @file ethernet.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"
#include <stdint.h>

typedef void (*ethernet_protocol_t)(void*, int);

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
int ethernet_send_packet(uint8_t* dst_mac_addr, uint8_t* data, uint32_t len, uint16_t protocol);

/**
 * @brief Handle inbound packet via interrupt from network card driver
 * 
 * @param packet raw packet data
 * @param len packet data length
 */
void ethernet_handle_packet(ethernet_frame_t * packet, int len);

/**
 * @brief Register a protocol with the ethernet layer
 * See https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
 * for a list of protocol numbers
 * 
 * @param protocol_number IEE802 protocol number to register
 * @param handler protocol handler
 * @return true if registered, false if there was an error
 */
bool ethernet_register_iee802_number(uint16_t protocol_number, ethernet_protocol_t handler);