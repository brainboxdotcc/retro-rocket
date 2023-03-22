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
	unsigned char	id[2];		/* Request id */
	unsigned int	flags1;		/* Flags */
	unsigned int	flags2;		/* Flags */
	unsigned int	qdcount;
	unsigned int	ancount;	/* Answer count */
	unsigned int	nscount;	/* Nameserver count */
	unsigned int	arcount;
	unsigned char	payload[512];	/* Packet payload */
} __attribute__((packed)) dns_header_t;

typedef struct dns_request {
	unsigned char id[2]; /* Request id */
	uint32_t rr_class; /* Request class */
	uint8_t type; /* Request type */
	unsigned char* orig; /* Original requested name/ip */
} dns_request_t;

#define DNS_DST_PORT 53
#define DNS_SRC_PORT 7921

int dns_lookup_host(uint32_t resolver_ip, const char* hostname);

void init_dns();