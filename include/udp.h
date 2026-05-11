/**
 * @file udp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

#include "kernel.h"

/**
 * @brief UDP packet receive callback
 *
 * Called when a UDP datagram is received for a registered destination port.
 *
 * @param src_ip Source IPv4 address in host byte order
 * @param src_port Source UDP port
 * @param dst_port Destination UDP port
 * @param data UDP payload data
 * @param len UDP payload length in octets
 * @param opaque Opaque pointer supplied during daemon registration
 */
typedef void (*udp_daemon_handler)(uint32_t, uint16_t, uint16_t, void*, uint32_t, void* opaque);

/**
 * @brief Raw UDP packet header and payload
 */
typedef struct udp_packet {
	/** Source UDP port */
	uint16_t src_port;

	/** Destination UDP port */
	uint16_t dst_port;

	/** UDP packet length including header */
	uint16_t length;

	/** UDP checksum, optional in IPv4 */
	uint16_t checksum;

	/** UDP payload */
	uint8_t data[];
} __attribute__((packed)) udp_packet_t;

/**
 * @brief Calculate the checksum for a UDP packet
 *
 * IPv4 permits a UDP checksum of zero to indicate that no checksum is present.
 * Retro Rocket currently transmits UDP packets with checksum disabled and this
 * function therefore always returns zero.
 *
 * @param packet UDP packet data
 * @return uint16_t UDP checksum value
 */
uint16_t udp_calculate_checksum(udp_packet_t* packet);

/**
 * @brief Send a UDP packet
 *
 * Builds a UDP datagram and transmits it through the IPv4 layer.
 *
 * @param dst_ip Destination IPv4 address
 * @param src_port Source UDP port
 * @param dst_port Destination UDP port
 * @param data UDP payload data
 * @param len UDP payload length in octets
 */
void udp_send_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, uint16_t len);

/**
 * @brief Register a UDP daemon handler
 *
 * Associates a callback with a destination UDP port. If dst_port is zero, an
 * ephemeral port greater than or equal to 1024 is allocated automatically.
 *
 * @param dst_port Destination UDP port to bind to, or zero for automatic allocation
 * @param handler Callback invoked for inbound UDP datagrams
 * @param opaque Opaque pointer passed through to the callback
 * @return uint16_t Allocated or bound UDP port, or zero on failure
 */
uint16_t udp_register_daemon(uint16_t dst_port, udp_daemon_handler handler, void* opaque);

/**
 * @brief Unregister a UDP daemon handler
 *
 * Removes a previously registered UDP daemon callback from a destination port.
 *
 * @param dst_port Destination UDP port
 * @param handler Handler previously registered on the port
 * @return true if a daemon was removed
 * @return false if no matching daemon existed
 */
bool udp_unregister_daemon(uint16_t dst_port, udp_daemon_handler handler);

/**
 * @brief Initialise the UDP subsystem
 *
 * Registers UDP with the IPv4 protocol dispatcher.
 */
void udp_init();