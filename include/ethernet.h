/**
 * @file ethernet.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
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
 * @brief Ethernet II frame header visible to the network stack
 *
 * Real Ethernet frames also contain a preamble, start frame delimiter,
 * frame check sequence and inter-frame gap. Those fields are handled by
 * the NIC/MAC layer and are not normally present in the packet buffer
 * delivered to software.
 */
typedef struct ethernet_frame {
	/** Destination MAC address */
	uint8_t dst_mac_addr[6];

	/** Source MAC address */
	uint8_t src_mac_addr[6];

	/** EtherType field */
	uint16_t type;

	/** Ethernet payload */
	uint8_t data[];
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
int ethernet_send_packet(const uint8_t* dst_mac_addr, const uint8_t* data, size_t len, uint16_t protocol);

/**
 * @brief Handle inbound packet via interrupt from network card driver
 * 
 * @param packet raw packet data
 * @param len packet data length
 */
void ethernet_handle_packet(ethernet_frame_t * packet, size_t len);

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