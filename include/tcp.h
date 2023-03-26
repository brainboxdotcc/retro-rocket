#pragma once

#include "kernel.h"

#define TCP_WINDOW_SIZE		8192
#define TCP_PACKET_SIZE_OFF	5

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
	TCP_LISTEN		= 0,
	TCP_SYN_SENT		= 1,
	TCP_SYN_RECEIVED	= 2,
	TCP_ESTABLISHED		= 3,
	TCP_FIN_WAIT_1		= 4,
	TCP_FIN_WAIT_2		= 5,
	TCP_CLOSE_WAIT		= 6,
	TCP_CLOSING		= 7,
	TCP_LAST_ACK		= 8,
	TCP_TIME_WAIT		= 9,
} tcp_state_t;

typedef enum tcp_error_t {
	TCP_CONN_RESET		= 1,
	TCP_CONN_REFUSED	= 2,
	TCP_CONN_CLOSING	= 3,
} tcp_error_t;

typedef struct tcp_ordered_list_t {
	tcp_segment_t* segment;
	size_t len;
	struct tcp_ordered_list_t* prev;
	struct tcp_ordered_list_t* next;
} tcp_ordered_list_t;

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

	uint32_t msl_time;	// Maximum socket lifetime timeout or 0

	tcp_ordered_list_t* segment_list;
} tcp_conn_t;

typedef enum tcp_port_type_t {
	TCP_PORT_LOCAL,
	TCP_PORT_REMOTE,
} tcp_port_type_t;

typedef enum tcp_error_code_t {
	TCP_ERROR_ALREADY_CLOSING = -1,
	TCP_ERROR_PORT_IN_USE = -2,
	TCP_ERROR_NETWORK_DOWN = -3,
	TCP_ERROR_INVALID_CONNECTION = -4,
	TCP_ERROR_WRITE_TOO_LARGE = -5,
} tcp_error_code_t;

void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len);

/**
 * @brief Initialise TCP protocol, must happen after IP
 */
void tcp_init();

/**
 * @brief Connect to a TCP port at a given IPv4 address.
 * 
 * @param target_addr Target address to connect to
 * @param target_port Target port to connect to
 * @param source_port Our source port to use, or 0 to choose automatically
 * @return Zero on success, error code on error
 */
int tcp_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port);

/**
 * @brief Close a TCP connection
 * 
 * @param conn 
 * @return zero on success, error code on error
 */
int tcp_close(tcp_conn_t* conn);

/**
 * @brief Write to TCP connection.
 * This function will only accept data up to TCP_WINDOW_SIZE.
 * 
 * @param conn Existing established connection
 * @param data Data to write
 * @param count Size of data to send
 * @return int Zero on success, error code on error
 */
int tcp_write(tcp_conn_t* conn, const void* data, size_t count);

void tcp_idle();