#pragma once

#include "kernel.h"

#define TCP_WINDOW_SIZE		8192

// checksummed part of ip segment
typedef struct tcp_checksummed_t
{
	uint32_t src;
	uint32_t dst;
	uint8_t reserved;
	uint8_t protocol;
	uint16_t len;
} __attribute__((packed)) tcp_checksummed_t;

typedef struct tcp_segment {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq;
	uint32_t ack;
	uint8_t off;
	uint8_t flags;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urgent;
	uint8_t options[0];
	uint8_t payload[];
} __attribute__((packed)) tcp_segment_t;


enum tcp_flags_t {
	TCP_FIN		= (1 << 0),
	TCP_SYN		= (1 << 1),
	TCP_RST		= (1 << 2),
	TCP_PSH		= (1 << 3),
	TCP_ACK		= (1 << 4),
	TCP_URG		= (1 << 5),
};

enum tcp_opt_t {
	TCP_OPT_END,
	TCP_OPT_NOP,
	TCP_OPT_MSS,
};

enum tcp_state_t {
	TCP_CLOSED		= 0,
	TCP_LISTEN		= 1,
	TCP_SYN_SENT		= 2,
	TCP_SYN_RECEIVED	= 3,
	TCP_ESTABLISHED		= 4,
	TCP_FIN_WAIT_1		= 5,
	TCP_FIN_WAIT_2		= 6,
	TCP_CLOSE_WAIT		= 7,
	TCP_CLOSING		= 8,
	TCP_LAST_ACK		= 9,
	TCP_TIME_WAIT		= 10,
};

enum tcp_error_t {
	TCP_CONN_RESET		= 1,
	TCP_CONN_REFUSED	= 2,
	TCP_CONN_CLOSING	= 3,
};

typedef struct tcp_conn_t
{
	uint8_t state;

	uint32_t local_addr;
	uint32_t remote_addr;
	uint16_t local_port;
	uint16_t remote_port;

	// send state
	uint32_t snd_una;	// send unacknowledged
	uint32_t snd_nxt;	// send next
	uint32_t snd_wnd;	// send window
	uint32_t snd_up;	// send urgent pointer
	uint32_t snd_wl1;	// segment sequence number used for last window update
	uint32_t snd_wl2;	// segment acknowledgment number used for last window update
	uint32_t iss;		// initial send sequence number

	// receive state
	uint32_t rcv_nxt;	// receive next
	uint32_t rcv_wnd;	// receive window
	uint32_t rcv_up;	// receive urgent pointer
	uint32_t irs;		// initial receive sequence number
} tcp_conn_t;

