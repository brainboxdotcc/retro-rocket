/**
 * @file dns.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 *
 * @brief Non-blocking DNS resolver support
 */
#pragma once

#include "kernel.h"

/**
 * @brief DNS compression pointer marker bits
 *
 * DNS names may use a two-byte compression pointer. The top two bits are set
 * to indicate that the remaining fourteen bits are an offset into the DNS
 * message.
 */
#define DN_COMP_BITMASK 0xC000

/**
 * @brief Resolver result flag indicating failure
 */
#define ERROR_MASK 0x10000

/**
 * @brief DNS recursion desired flag
 */
#define FLAGS_MASK_RD 0x01

/**
 * @brief DNS truncated response flag
 */
#define FLAGS_MASK_TC 0x02

/**
 * @brief DNS authoritative answer flag
 */
#define FLAGS_MASK_AA 0x04

/**
 * @brief DNS opcode mask
 */
#define FLAGS_MASK_OPCODE 0x78

/**
 * @brief DNS query/response flag
 */
#define FLAGS_MASK_QR 0x80

/**
 * @brief DNS response code mask
 */
#define FLAGS_MASK_RCODE 0x0F

/**
 * @brief DNS reserved flag mask
 */
#define FLAGS_MASK_Z 0x70

/**
 * @brief DNS recursion available flag
 */
#define FLAGS_MASK_RA 0x80

/**
 * @brief Returned by dns_lookup_host_async() when the result is cached
 *
 * This value is returned if the result is immediately available in the ip
 * parameter, and the ip parameter was non-NULL.
 */
#define DNS_RESULT_CACHED 0xFFFFFFFF

/**
 * @brief DNS query and resource record types
 */
enum query_type_t {
	/** Uninitialised query */
	DNS_QUERY_NONE = 0,

	/** A record: an IPv4 address */
	DNS_QUERY_A = 1,

	/** CNAME record: an alias to another hostname */
	DNS_QUERY_CNAME = 5,

	/** PTR record: a reverse lookup hostname */
	DNS_QUERY_PTR = 12,

	/** AAAA record: an IPv6 address */
	DNS_QUERY_AAAA = 28,

	/** Force PTR to use IPv4 semantics */
	DNS_QUERY_PTR4 = 0xFFFD,

	/** Force PTR to use IPv6 semantics */
	DNS_QUERY_PTR6 = 0xFFFE
};

/**
 * @brief Size of the fixed DNS packet header
 */
#define DNS_HEADER_SIZE 12

/**
 * @brief Size of the fixed part of a DNS resource record after the name
 *
 * This covers TYPE, CLASS, TTL, and RDLENGTH. It does not include the owner
 * name or RDATA.
 */
#define DNS_RR_FIXED_SIZE 10

/**
 * @brief Maximum decoded DNS result size used by the resolver
 */
#define DNS_RESULT_MAX 1023

/**
 * @brief Maximum number of CNAME redirects followed for one request
 */
#define DNS_MAX_CNAME_DEPTH 8

/**
 * @brief Standard DNS server UDP port
 */
#define DNS_DST_PORT 53

/**
 * @brief DNS request/reply header with opaque packet payload
 */
typedef struct dns_header {
	/** Request or reply identifier */
	uint16_t id;

	/** First DNS flags byte */
	uint8_t flags1;

	/** Second DNS flags byte */
	uint8_t flags2;

	/** Question count */
	uint16_t qdcount;

	/** Answer count */
	uint16_t ancount;

	/** Authority resource record count */
	uint16_t nscount;

	/** Additional resource record count */
	uint16_t arcount;

	/** Packet payload following the fixed DNS header */
	uint8_t payload[512];
} __attribute__((packed)) dns_header_t;

/**
 * @brief Callback for completed IPv4 A lookups
 *
 * @param result IPv4 address result, or 0 on failure
 * @param hostname Original hostname supplied to the lookup
 * @param reply_id DNS request identifier
 */
typedef void (*dns_reply_callback_a)(uint32_t result, const char* hostname, uint16_t reply_id);

/**
 * @brief Callback for completed IPv6 AAAA lookups
 *
 * @param result IPv6 address result
 * @param hostname Original hostname supplied to the lookup
 * @param reply_id DNS request identifier
 */
typedef void (*dns_reply_callback_aaaa)(uint8_t* result, const char* hostname, uint16_t reply_id);

/**
 * @brief Callback for completed PTR lookups
 *
 * @param result Hostname result
 * @param ip Original IP address supplied to the lookup
 * @param reply_id DNS request identifier
 */
typedef void (*dns_reply_callback_ptr)(const char* const result, const uint32_t ip, uint16_t reply_id);

/**
 * @brief Pending DNS request state
 */
typedef struct dns_request {
	/** Request identifier */
	uint16_t id;

	/** Requested resource record class */
	uint32_t rr_class;

	/** Time to live from the accepted resource record */
	uint32_t ttl;

	/** Requested resource record type */
	uint16_t type;

	/** Original requested hostname or IP string */
	unsigned char* orig;

	/** Result buffer for resolved binary or textual data */
	char result[256];

	/** Number of valid bytes in result */
	uint8_t result_length;

	/** Completion callback for A lookups */
	dns_reply_callback_a callback_a;

	/** Completion callback for AAAA lookups */
	dns_reply_callback_aaaa callback_aaaa;

	/** Completion callback for PTR lookups */
	dns_reply_callback_ptr callback_ptr;

	/** Resolver IPv4 address */
	uint32_t resolver_ip;

	/** Number of CNAME redirects followed by this request */
	uint8_t cname_depth;
} dns_request_t;

/**
 * @brief Cached DNS lookup entry
 */
typedef struct dns_cache_entry_t {
	/** Cached hostname */
	const char* host;

	/** Cached result string */
	const char* result;
} dns_cache_entry_t;

/**
 * @brief DNS resource record metadata
 */
typedef struct resource_record {
	/** Resource record type */
	uint16_t type;

	/** Resource record class */
	uint16_t rr_class;

	/** Time to live */
	uint32_t ttl;

	/** Resource data length */
	uint16_t rdlength;
} __attribute__((packed)) resource_record_t;

/**
 * @brief DNS resolver result metadata
 */
struct dns_result_t {
	/** Error string, or NULL if no error occurred */
	char* error;
};

/**
 * @brief Look up an IPv4 hostname to IP address, with timeout
 *
 * @note This function is synchronous. It blocks until a suitable DNS response
 * has been obtained from the specified DNS server, or a timeout is reached.
 *
 * @param resolver_ip IP address of the resolver to use, in network byte order
 * @param hostname Hostname to resolve
 * @param timeout_ms Timeout in milliseconds
 * @return Resolved IP address. On error or timeout, the return value is 0,
 * which represents 0.0.0.0.
 */
uint32_t dns_lookup_host(uint32_t resolver_ip, const char* hostname, uint32_t timeout_ms);

/**
 * @brief Look up an IPv4 hostname to IP address asynchronously
 *
 * @note This function returns the ID of the submitted request. The callback is
 * called once the request has completed. If the result is already cached and ip
 * is non-NULL, DNS_RESULT_CACHED is returned and ip is filled immediately.
 *
 * @param resolver_ip IP address of the resolver to use, in network byte order
 * @param hostname Hostname to resolve
 * @param ip Optional pointer to receive an immediate cached result
 * @param callback Callback to receive the resolved IP address
 * @return Request ID that was submitted, 0 on error, or DNS_RESULT_CACHED if
 * the result was returned from cache.
 */
uint32_t dns_lookup_host_async(uint32_t resolver_ip, const char* hostname, uint32_t* ip, dns_reply_callback_a callback);

/**
 * @brief Initialise DNS protocol support
 *
 * This binds a UDP port for use with DNS replies.
 */
void init_dns();