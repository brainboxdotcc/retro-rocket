/**
 * @file dns.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

/* highest 6 bits in a DN label header */
#define DN_COMP_BITMASK	0xC000

/* Result is an error */
#define ERROR_MASK 0x10000

#define FLAGS_MASK_RD 0x01	/* Recursive */
#define FLAGS_MASK_TC 0x02
#define FLAGS_MASK_AA 0x04	/* Authoritative */
#define FLAGS_MASK_OPCODE 0x78
#define FLAGS_MASK_QR 0x80
#define FLAGS_MASK_RCODE 0x0F	/* Request */
#define FLAGS_MASK_Z 0x70
#define FLAGS_MASK_RA 0x80

enum query_type_t {
	/** Uninitialized Query */
	DNS_QUERY_NONE	= 0,
	/** 'A' record: an ipv4 address */
	DNS_QUERY_A	= 1,
	/** 'CNAME' record: An alias */
	DNS_QUERY_CNAME	= 5,
	/** 'PTR' record: a hostname */
	DNS_QUERY_PTR	= 12,
	/** 'AAAA' record: an ipv6 address */
	DNS_QUERY_AAAA	= 28,

	/** Force 'PTR' to use IPV4 scemantics */
	DNS_QUERY_PTR4	= 0xFFFD,
	/** Force 'PTR' to use IPV6 scemantics */
	DNS_QUERY_PTR6	= 0xFFFE
};

/** Represents a dns resource record (rr)
 */
struct ResourceRecord
{
	uint8_t type;		/* Record type */
	uint32_t rr_class;	/* Record class */
	uint32_t ttl;		/* Time to live */
	uint32_t rdlength;	/* Record length */
};

/** Represents a dns request/reply header, and its payload as opaque data.
 */
typedef struct dns_header {
	uint16_t id;		/* Request id */
	uint8_t flags1;		/* Flags */
	uint8_t flags2;		/* Flags */
	uint16_t qdcount;
	uint16_t ancount;	/* Answer count */
	uint16_t nscount;	/* Nameserver count */
	uint16_t arcount;
	uint8_t payload[512];	/* Packet payload */
} __attribute__((packed)) dns_header_t;

typedef void (*dns_reply_callback_a)(uint32_t result, const char* hostname, uint16_t reply_id);
typedef void (*dns_reply_callback_aaaa)(uint8_t* result, const char* hostname, uint16_t reply_id);
typedef void (*dns_reply_callback_ptr)(const char* const result, const uint32_t ip, uint16_t reply_id);

typedef struct dns_request {
	uint16_t id; /* Request id */
	uint32_t rr_class; /* Request class */
	uint32_t ttl;
	uint16_t type; /* Request type */
	unsigned char* orig; /* Original requested name/ip */
	char result[256];
	uint8_t result_length;
	dns_reply_callback_a callback_a; /* For later */
	dns_reply_callback_aaaa callback_aaaa; /* For later */
	dns_reply_callback_ptr callback_ptr; /* For later */
} dns_request_t;

typedef struct resource_record {
	uint16_t type;		/* Record type */
	uint16_t rr_class;	/* Record class */
	uint32_t ttl;		/* Time to live */
	uint16_t rdlength;	/* Record length */
} __attribute__((packed)) resource_record_t;

struct dns_result_t {
	char* error;
};

#define DNS_DST_PORT 53

/**
 * @brief Look up an IPV4 hostname to IP address, with timeout
 * @note This function is synchronous! It will block until a suitable DNS request
 * has been obtained from the DNS server you specify, or a timeout is reached.
 * 
 * @param resolver_ip The IP of the resolver to use, in network byte order
 * @param hostname Host address to resolve
 * @param timeout Timeout in seconds
 * @return uint32_t Resolved IP address. On error or timeout, the return value is 0,
 * which translates to 0.0.0.0.
 */
uint32_t dns_lookup_host(uint32_t resolver_ip, const char* hostname, uint32_t timeout);

/**
 * @brief Look up an IPV4 hostname to IP address, with timeout
 * @note This function is asynchronous. It will return the ID of the request,
 * and will call the callback once the request has completed.
 * 
 * @param resolver_ip The IP of the resolver to use, in network byte order
 * @param hostname Host address to resolve
 * @param timeout Timeout in seconds
 * @param dns_reply_callback_a Callback to receive the resolved IP address.
 * If an error occured during resolution, the received IP address will be 0,
 * which is a representation of 0.0.0.0.
 * @return uint16_t Request ID that was submitted, or 0 on error
 */
uint16_t dns_lookup_host_async(uint32_t resolver_ip, const char* hostname, uint32_t timeout, dns_reply_callback_a callback);

/**
 * @brief Initialise DNS protocol.
 * This binds a UDP port for use with replies.
 */
void init_dns();