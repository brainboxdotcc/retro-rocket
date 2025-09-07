/**
 * @file arp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 * @brief ARP (Address Resolution Protocol) implementation for Retro Rocket
 */
#pragma once

#include "kernel.h"

#define ETHERNET_TYPE_ARP 0x0806  ///< Ethernet frame type for ARP

/**
 * @brief Opcode values for ARP packets
 */
enum arp_packet_type_t {
	ARP_REQUEST = 1,  ///< ARP Request
	ARP_REPLY   = 2,  ///< ARP Reply
};

/**
 * @brief Format of an ARP packet (per RFC 826)
 */
typedef struct arp_packet {
	uint16_t hardware_type;         ///< Type of hardware (1 = Ethernet)
	uint16_t protocol;              ///< Protocol type (0x0800 = IPv4)
	uint8_t  hardware_addr_len;     ///< Length of hardware address (6 for MAC)
	uint8_t  protocol_addr_len;     ///< Length of protocol address (4 for IPv4)
	uint16_t opcode;                ///< Operation code (ARP_REQUEST or ARP_REPLY)
	uint8_t  src_hardware_addr[6];  ///< Sender MAC address
	uint8_t  src_protocol_addr[4];  ///< Sender IP address
	uint8_t  dst_hardware_addr[6];  ///< Target MAC address
	uint8_t  dst_protocol_addr[4];  ///< Target IP address
} __attribute__((packed)) arp_packet_t;

/**
 * @brief A single ARP table entry
 */
typedef struct arp_table_entry {
	uint32_t ip_addr;    ///< IPv4 address (in host byte order)
	uint64_t mac_addr;   ///< MAC address (6-byte value stored in low 48 bits)
} arp_table_entry_t;

/**
 * @brief Send an ARP packet via the Ethernet driver
 *
 * @param dst_hardware_addr Destination MAC address
 * @param dst_protocol_addr Destination IPv4 address
 */
void arp_send_packet(uint8_t *dst_hardware_addr, uint8_t *dst_protocol_addr);

/**
 * @brief Perform a blocking ARP lookup to resolve an IP address
 *
 * @param ret_hardware_addr Output buffer for resolved MAC address
 * @param ip_addr IPv4 address to resolve
 * @return int Non-zero on success, zero on failure or timeout
 */
int arp_lookup(uint8_t *ret_hardware_addr, uint8_t *ip_addr);

/**
 * @brief Manually add an ARP entry to the local cache
 *
 * @param ret_hardware_addr MAC address to associate
 * @param ip_addr IPv4 address to associate
 */
void arp_lookup_add(uint8_t *ret_hardware_addr, uint8_t *ip_addr);

/**
 * @brief Initialise the ARP subsystem
 */
void arp_init();

/**
 * @brief Retrieve a pointer to an ARP table entry by index
 *
 * Primarily used by the BASIC interpreter for table enumeration.
 *
 * @param index Table index
 * @return arp_table_entry_t* Pointer to ARP entry, or NULL if out of range
 */
arp_table_entry_t* get_arp_entry(size_t index);

/**
 * @brief Get the current number of ARP table entries
 *
 * Used by the BASIC interpreter to determine iteration limits.
 *
 * @return size_t Number of valid entries in the ARP table
 */
size_t get_arp_table_size();

/**
 * @brief Pre-cache a likely-to-be-used IP address in the ARP table
 *
 * Used to resolve IPs such as the gateway or DNS server early,
 * avoiding runtime lookup delays.
 *
 * @param protocol_addr IP address to resolve and cache
 */
void arp_prediscover(uint8_t *protocol_addr);

/* Broadcast (announce) our current IPv4 address via gratuitous ARP.
 * Safe to call immediately after assigning a static IP. No-op if no IP set.
 */
void arp_announce_my_ip(void);
