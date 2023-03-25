#pragma once

#include "kernel.h"

#define TCP_WINDOW_SIZE		8192

// checksummed part of ip segment
typedef struct tcp_ip_pseudo_header_t
{
	uint32_t src;
	uint32_t dst;
	uint8_t reserved;
	uint8_t protocol;
	uint16_t len;
	uint8_t body[];
} __attribute__((packed)) tcp_ip_pseudo_header_t;

typedef union tcp_segment_flags_t {
	uint8_t bits1;
	uint8_t bits2;
	struct {
		uint8_t reserved:4;
		uint8_t off:4;
		uint8_t fin:1;
		uint8_t syn:1;
		uint8_t rst:1;
		uint8_t psh:1;
		uint8_t ack:1;
		uint8_t urg:1;
		uint8_t ece:1;
		uint8_t cwr:1;
	};
} __attribute__((packed)) tcp_segment_flags_t;

typedef enum {
	TCP_FIN = 1,
	TCP_SYN = 2,
	TCP_RST = 4,
	TCP_PSH = 8,
	TCP_ACK = 16,
	TCP_URG = 32,
	TCP_ECE = 64,
	TCP_CWR = 128,
} tcp_state_flags_t;

typedef struct tcp_segment {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq;
	uint32_t ack;
	tcp_segment_flags_t flags;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urgent;
	uint8_t options[0];
	uint8_t payload[];
} __attribute__((packed)) tcp_segment_t;


enum tcp_opt_t {
	TCP_OPT_END = 0,
	TCP_OPT_NOP = 1,
	TCP_OPT_MSS = 2,
};

typedef struct tcp_options_t {
	uint16_t mss;
} tcp_options_t;

typedef enum tcp_state_t {
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
} tcp_state_t;

typedef enum tcp_error_t {
	TCP_CONN_RESET		= 1,
	TCP_CONN_REFUSED	= 2,
	TCP_CONN_CLOSING	= 3,
} tcp_error_t;

typedef struct tcp_conn_t
{
	tcp_state_t state;

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

void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len);

void tcp_init();

tcp_conn_t* tcp_connect(uint32_t target_addr, uint16_t target_port);