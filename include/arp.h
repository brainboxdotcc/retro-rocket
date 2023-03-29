#pragma once

#include "kernel.h"

#define ETHERNET_TYPE_ARP 0x0806

enum arp_packet_type_t {
	ARP_REQUEST = 1,
	ARP_REPLY = 2,
};

typedef struct arp_packet {
	uint16_t hardware_type;
	uint16_t protocol;
	uint8_t hardware_addr_len;
	uint8_t protocol_addr_len;
	uint16_t opcode;
	uint8_t src_hardware_addr[6];
	uint8_t src_protocol_addr[4];
	uint8_t dst_hardware_addr[6];
	uint8_t dst_protocol_addr[4];
} __attribute__((packed)) arp_packet_t;

typedef struct arp_table_entry {
	uint32_t ip_addr;
	uint64_t mac_addr;
} arp_table_entry_t;

/**
 * @brief Send an ARP packet to the ethernet driver
 * 
 * @param dst_hardware_addr destination MAC address
 * @param dst_protocol_addr destination IP address
 */
void arp_send_packet(uint8_t * dst_hardware_addr, uint8_t * dst_protocol_addr);

/**
 * @brief Lookup a MAC address for an IP address via a blocking ARP request
 * 
 * @param ret_hardware_addr MAC address (filled on return)
 * @param ip_addr IP address to request
 * @return int nonzero if ARP request succeeded
 */
int arp_lookup(uint8_t * ret_hardware_addr, uint8_t * ip_addr);

/**
 * @brief Cache and ARP entry in the ARP cache
 * 
 * @param ret_hardware_addr MAC address
 * @param ip_addr IP address 
 */
void arp_lookup_add(uint8_t * ret_hardware_addr, uint8_t * ip_addr);

/**
 * @brief Initialise the ARP protocol
 */
void arp_init();

/**
 * @brief Get an arp entry object, used by the BASIC interpreter
 * 
 * @param index index to find
 * @return arp_table_entry_t* ARP entry 
 */
arp_table_entry_t* get_arp_entry(size_t index);

/**
 * @brief Get the arp table size, used by the BASIC interpreter
 * 
 * @return size_t the highest index in the ARP cache plus one
 */
size_t get_arp_table_size();

/**
 * @brief Precache a local network IP address, to save on doing an
 * ARP lookup later when the user wants to access the resource.
 * Used for the DNS server and gateway addresses.
 */
void arp_prediscover(uint8_t* protocol_addr);