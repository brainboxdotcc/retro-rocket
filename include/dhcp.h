#pragma once

#include "kernel.h"

enum dhcp_request_type {
	DHCP_REQUEST = 1,
	DHCP_REPLY  = 2,
};

#define DHCP_TRANSACTION_IDENTIFIER 0x55AA55AA

/**
 * @brief Definition of a DHCP packet
 */
typedef struct dhcp_packet {
	uint8_t op;
	uint8_t hardware_type;
	uint8_t hardware_addr_len;
	uint8_t hops;
	uint32_t xid;
	uint16_t seconds;
	uint16_t flags;
	uint32_t client_ip; // our IP, empty unless renewing
	uint32_t your_ip; // IP allocated by DHCP server
	uint32_t server_ip; // Server's IP, must be filled when DHCPREQUEST
	uint32_t relay_agent_ip; 
	uint8_t client_hardware_addr[16];
	uint8_t server_name[64];
	uint8_t boot_file[128];
	uint8_t options[64];
} __attribute__ ((packed)) dhcp_packet_t;

/**
 * @brief Initiate DHCP discovery
 * Non-blocking, establishes IP address in the background.
 * Network and IP stack card must be initialised first.
 */
void dhcp_discover();

/**
 * @brief send DHCPREQUEST packet.
 * We send one on dhcp_discover() as DHCPDISCOVER then a second one
 * as DHCPREQUEST once we are offered an IP address.
 * 
 * @param request_ip IP address we have been offered, if we have been
 * offered an IP yet.
 * @param xid request ID as set by us when we did DHCPDISCOVER
 * @param server_ip server IP that sent the offer to us
 */
void dhcp_request(uint8_t* request_ip, uint32_t xid, uint32_t server_ip);

/**
 * @brief Handle DHCP packet from the UDP driver
 * 
 * @param packet Raw DHCP packet
 * @param length DHCP packet length
 */
void dhcp_handle_packet(dhcp_packet_t* packet, size_t length);

/**
 * @brief Get an option from a DHCP packet
 * 
 * @param packet raw packet
 * @param type type to search for
 * @return void* raw option, size must be known to the caller!
 */
void* get_dhcp_options(dhcp_packet_t* packet, uint8_t type);

/**
 * @brief Build a DHCP packet.
 * The contents of the options are built dynamically.
 * 
 * @param packet Packet structure to fill
 * @param msg_type Message type to build
 * @param request_ip IP address we were offered
 * @param xid Our request identifier
 * @param server_ip IP of server that offered the configuration
 */
void make_dhcp_packet(dhcp_packet_t* packet, uint8_t msg_type, uint8_t* request_ip, uint32_t xid, uint32_t server_ip);
