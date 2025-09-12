/**
 * @file tcp.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

#define TCP_WINDOW_SIZE		65535
#define TCP_PACKET_SIZE_OFF	5

/* Set this to output or record a trace of the TCP I/O. This is very noisy! */
#define TCP_TRACE

/**
 * @brief IPv4 TCP pseudo-header used during checksum calculation.
 *
 * The TCP checksum is computed over the TCP header and payload plus this pseudo-header.
 * All fields are in network byte order; the structure is packed to match on-the-wire layout.
 */
typedef struct tcp_ip_pseudo_header_t
{
	uint32_t src;          /**< Source IPv4 address (network byte order) */
	uint32_t dst;          /**< Destination IPv4 address (network byte order) */
	uint8_t reserved;      /**< Must be zero */
	uint8_t protocol;      /**< IP protocol number (6 for TCP) */
	uint16_t len;          /**< Length of TCP header + payload (network byte order) */
	uint8_t body[];        /**< TCP header and payload follow for checksum computation */
} __attribute__((packed)) tcp_ip_pseudo_header_t;

/**
 * @brief TCP header flag field representation.
 *
 * This union allows access as raw bytes or as individual flag bits. The @ref off field
 * is the header length in 32-bit words (i.e., data offset); options begin at (off*4).
 * The layout follows the standard TCP header bit order.
 */
typedef union tcp_segment_flags_t {
	uint8_t bits1;         /**< Raw first flags byte (offset + reserved nibble) */
	uint8_t bits2;         /**< Raw second flags byte (control flags) */
	struct {
		uint8_t reserved:4; /**< Reserved (must be zero when sending) */
		uint8_t off:4;      /**< Data offset (header length) in 32-bit words */
		uint8_t fin:1;      /**< FIN: sender has finished sending */
		uint8_t syn:1;      /**< SYN: synchronise sequence numbers (handshake) */
		uint8_t rst:1;      /**< RST: abort the connection */
		uint8_t psh:1;      /**< PSH: push function (prompt delivery to app) */
		uint8_t ack:1;      /**< ACK: acknowledgement field is significant */
		uint8_t urg:1;      /**< URG: urgent pointer field is significant */
		uint8_t ece:1;      /**< ECE: ECN-Echo */
		uint8_t cwr:1;      /**< CWR: Congestion Window Reduced */
	};
} __attribute__((packed)) tcp_segment_flags_t;

/**
 * @brief Convenience bitmask values for building TCP control flags.
 */
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

/**
 * @brief TCP header as parsed/constructed by the stack (packed on-wire layout).
 *
 * Ports and sequence/ack numbers are in host byte order when stored in this struct
 * after byte-order conversion at the ingress/egress boundaries.
 */
typedef struct tcp_segment {
	uint16_t src_port;             /**< Source TCP port (host byte order within the stack) */
	uint16_t dst_port;             /**< Destination TCP port (host byte order within the stack) */
	uint32_t seq;                  /**< Sequence number */
	uint32_t ack;                  /**< Acknowledgement number */
	tcp_segment_flags_t flags;     /**< Flags and data offset */
	uint16_t window_size;          /**< Advertised receive window */
	uint16_t checksum;             /**< TCP checksum (network order on the wire) */
	uint16_t urgent;               /**< Urgent pointer (if URG set) */
	uint8_t options[0];            /**< Start of variable-length options (if any) */
	uint8_t payload[];             /**< Start of payload */
} __attribute__((packed)) tcp_segment_t;

/**
 * @brief Known TCP option kinds used by this implementation.
 */
enum tcp_opt_t {
	TCP_OPT_END = 0,               /**< End of option list */
	TCP_OPT_NOP = 1,               /**< No operation (padding) */
	TCP_OPT_MSS = 2,               /**< Maximum Segment Size (MSS) */
};

/**
 * @brief Parsed/advertised TCP options relevant to this implementation.
 */
typedef struct tcp_options_t {
	uint16_t mss;                  /**< Maximum Segment Size, or 0 if not present */
} tcp_options_t;

/**
 * @brief TCP connection state values (RFC 793/9293).
 */
typedef enum tcp_state_t {
	TCP_LISTEN		= 0,  /**< Listening for a connection */
	TCP_SYN_SENT		= 1,  /**< Active open: SYN sent */
	TCP_SYN_RECEIVED	= 2,  /**< Passive open: SYN received (SYN|ACK sent) */
	TCP_ESTABLISHED		= 3,  /**< Data transfer state */
	TCP_FIN_WAIT_1		= 4,  /**< Our FIN sent, waiting for ACK or peer FIN */
	TCP_FIN_WAIT_2		= 5,  /**< Our FIN acked, waiting for peer FIN */
	TCP_CLOSE_WAIT		= 6,  /**< Peer sent FIN; waiting for app close */
	TCP_CLOSING		= 7,  /**< Simultaneous close (both FINs seen, unacked) */
	TCP_LAST_ACK		= 8,  /**< Our FIN sent after peer FIN; waiting for ACK */
	TCP_TIME_WAIT		= 9,  /**< Timed wait before full close */
} tcp_state_t;

/**
 * @brief High-level connection error indications.
 */
typedef enum tcp_error_t {
	TCP_CONN_RESET		= 1,  /**< Connection reset (RST) */
	TCP_CONN_REFUSED	= 2,  /**< Connection refused */
	TCP_CONN_CLOSING	= 3,  /**< Connection is closing or closed */
} tcp_error_t;

/**
 * @brief Node in the out-of-order reassembly list, maintained in sequence order.
 *
 * Each node owns a copy of a TCP segment’s header and payload and tracks its payload length.
 */
typedef struct tcp_ordered_list_t {
	tcp_segment_t* segment;        /**< Owned copy of the TCP segment (header + payload) */
	size_t len;                    /**< Payload length (excludes header and options) */
	struct tcp_ordered_list_t* prev; /**< Previous node in sequence order */
	struct tcp_ordered_list_t* next; /**< Next node in sequence order */
} tcp_ordered_list_t;

/* Forward declaration for queue nodes and APIs. */
typedef struct tcp_conn_t tcp_conn_t;

/**
 * @brief Queue node for pending (half-open/ready) inbound connections.
 */
typedef struct pending_node {
	tcp_conn_t *conn;              /**< Connection control block pointer */
	struct pending_node *next;     /**< Next node (singly-linked) */
} pending_node_t;

/**
 * @brief FIFO queue of pending inbound connections for a listening socket.
 */
typedef struct {
	pending_node_t *head;          /**< Oldest pending connection */
	pending_node_t *tail;          /**< Newest pending connection */
} queue_t;

/**
 * @brief TCP connection control block (TCB).
 *
 * Holds the 4-tuple addressing, current TCP state, send/receive sequencers, negotiated windows,
 * high-level buffers for the POSIX-style wrapper, a reassembly list for out-of-order segments,
 * and bookkeeping for the pending-connection queue (LISTEN sockets).
 *
 * @note IP addresses are stored as host-order uint32_t within the TCB; conversion to/from network byte order happens at the protocol boundary.
 * @note Access to mutable fields is synchronised by @ref spinlock_t instances in the implementation where required.
 */
typedef struct tcp_conn_t
{
	tcp_state_t state;             /**< TCP FSM state */

	uint32_t local_addr;           /**< Local IPv4 address (host order) */
	uint32_t remote_addr;          /**< Remote IPv4 address (host order) */
	uint16_t local_port;           /**< Local TCP port (host order) */
	uint16_t remote_port;          /**< Remote TCP port (host order) */

	/* send state */
	uint32_t snd_una;              /**< Oldest unacknowledged sequence number */
	uint32_t snd_nxt;              /**< Next sequence number to send */
	uint32_t snd_wnd;              /**< Send window (peer’s advertised receive window) */
	uint32_t snd_up;               /**< Send urgent pointer */
	uint32_t snd_wl1;              /**< Segment.seq used for last window update */
	uint32_t snd_wl2;              /**< Segment.ack used for last window update */
	uint32_t iss;                  /**< Initial send sequence number */

	uint32_t snd_lst;              /**< Last sequence we ACKed (duplicate-ACK suppression) */
	uint32_t rcv_lst;              /**< Last rcv_nxt we ACKed (duplicate-ACK suppression) */

	/* receive state */
	uint32_t rcv_nxt;              /**< Next sequence number expected */
	uint32_t rcv_wnd;              /**< Receive window we are advertising */
	uint32_t rcv_up;               /**< Receive urgent pointer */
	uint32_t irs;                  /**< Initial receive sequence number */

	/* high level wrappers for POSIX style interface */
	int fd;                        /**< File descriptor (or -1 if none) */
	int recv_eof_pos;              /**< Position of EOF in recv buffer, or -1 if not seen */
	int send_eof_pos;              /**< Position of EOF in send buffer, or -1 if not set */
	uint8_t* recv_buffer;          /**< High-level receive buffer (owned) */
	size_t recv_buffer_len;        /**< Length of data in receive buffer (bytes) */
	uint8_t* send_buffer;          /**< High-level send buffer (owned) */
	size_t send_buffer_len;        /**< Length of data in send buffer (bytes) */

	spinlock_t recv_buffer_spinlock;/**< Lock guarding recv_buffer and length */
	spinlock_t send_buffer_spinlock;/**< Lock guarding send_buffer and length */

	uint32_t msl_time;             /**< TIME-WAIT expiry tick or 0 when not armed */

	tcp_ordered_list_t* segment_list; /**< Head of out-of-order reassembly list (may be NULL) */

	int backlog;                   /**< Backlog limit for LISTEN sockets (advisory) */

	queue_t* pending;              /**< Pending inbound connections for LISTEN sockets */
} tcp_conn_t;

/**
 * @brief Port types, either a local or remote port
 */
typedef enum tcp_port_type_t {
	TCP_PORT_LOCAL,                /**< Local (bind) port */
	TCP_PORT_REMOTE,               /**< Remote (peer) port */
} tcp_port_type_t;

/**
 * @brief Error codes which can be returned by socket functions
 */
typedef enum tcp_error_code_t {
	TCP_ERROR_ALREADY_CLOSING = -1,
	TCP_ERROR_PORT_IN_USE = -2,
	TCP_ERROR_NETWORK_DOWN = -3,
	TCP_ERROR_INVALID_CONNECTION = -4,
	TCP_ERROR_WRITE_TOO_LARGE = -5,
	TCP_ERROR_NOT_CONNECTED = -6,
	TCP_ERROR_OUT_OF_DESCRIPTORS = -7,
	TCP_ERROR_OUT_OF_MEMORY = -8,
	TCP_ERROR_INVALID_SOCKET = -9,
	TCP_ERROR_CONNECTION_FAILED = -10,
	TCP_ERROR_NOT_LISTENING = -11,
	TCP_ERROR_WOULD_BLOCK = -12,
	TCP_CONNECTION_TIMED_OUT = -13,
	TCP_LAST_ERROR = -14,
} tcp_error_code_t;

/**
 * @brief TCP handler called by the IP layer
 *
 * Entry point for inbound TCP segments from the IP layer. Performs checksum verification,
 * byte-order conversion, option parsing, TCB lookup, and dispatches to the TCP state machine.
 *
 * @param encap_packet Encapsulating IP packet (addresses in network order)
 * @param segment TCP segment buffer (will be converted to host order on entry)
 * @param len Length of TCP segment including header and options
 */
void tcp_handle_packet([[maybe_unused]] ip_packet_t* encap_packet, tcp_segment_t* segment, size_t len);

/**
 * @brief Initialise TCP protocol, must happen after IP
 *
 * Creates the global TCB hashmap and registers the TCP idle handler with the scheduler/ISR layer.
 */
void tcp_init();

/**
 * @brief Connect to a TCP port at a given IPv4 address.
 *
 * Performs an active open: allocates a local port if required, creates a TCB, sends a SYN,
 * and drives the state machine to await completion. If @p blocking is false, returns
 * immediately with a file descriptor for a socket in @ref TCP_SYN_SENT; the caller may
 * poll @ref is_connected() or wait on events.
 *
 * @param target_addr Target IPv4 address (host byte order)
 * @param target_port Target TCP port (host byte order)
 * @param source_port Local source port to use, or 0 to choose automatically
 * @param blocking If true, waits for establishment or error before returning
 * @return Non-negative file descriptor on success; negative @ref tcp_error_code_t on failure
 */
int connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port, bool blocking);

/**
 * @brief Close a TCP connection
 *
 * Initiates or completes connection teardown depending on the current state.
 *
 * @param socket Socket descriptor to close
 * @return 0 on success, negative @ref tcp_error_code_t on error
 */
int closesocket(int socket);

/**
 * @brief Send data to an open socket
 *
 * Appends data to the socket’s send buffer. Actual transmission is driven by the TCP idle
 * handler and/or state machine.
 *
 * @param socket Socket descriptor from @ref connect()
 * @param buffer Buffer to send
 * @param length Number of bytes to send
 * @return Number of bytes queued on success; negative @ref tcp_error_code_t on error
 */
int send(int socket, const void* buffer, uint32_t length);

/**
 * @brief Receive data from an open socket.
 *
 * Copies up to @p maxlen bytes from the socket’s receive buffer into @p buffer.
 * If @p blocking is true, waits up to @p timeout ticks for data to become available.
 *
 * @param socket Socket descriptor from @ref connect()
 * @param buffer Destination buffer
 * @param maxlen Maximum number of bytes to copy
 * @param blocking True to block until data is available, false for non-blocking
 * @param timeout Timeout (ticks) when blocking
 * @return Number of bytes received; negative @ref tcp_error_code_t on error
 */
int recv(int socket, void* buffer, uint32_t maxlen, bool blocking, uint32_t timeout);

/**
 * @brief Check if a socket has data ready to be read.
 *
 * Non-blocking check for buffered data on the given socket. Does not consume data.
 *
 * @param socket File descriptor
 * @return true if there is data available to read, false otherwise
 */
bool sock_ready_to_read(int socket);

/**
 * @brief Returns true if the socket is connected
 *
 * @param socket Socket descriptor from @ref connect()
 * @return true if socket is in @ref TCP_ESTABLISHED, false otherwise
 */
bool is_connected(int socket);

/**
 * @brief Return the error description associated with an error code
 * @note Invalid error codes return the constant "No error".
 *
 * @param error_code Error code (negative @ref tcp_error_code_t)
 * @return const char* Description string
 */
const char* socket_error(int error_code);

/**
 * @brief Idle loop called from timer ISR/scheduler to drive retransmit, send queues, and time-based events.
 */
void tcp_idle();

/**
 * @brief Allocate a new empty queue
 * @return queue_t* Newly allocated queue, or NULL on failure
 */
queue_t* queue_new(void);

/**
 * @brief Push a tcp_conn_t into the queue
 * @param q Queue
 * @param conn TCP connection to enqueue
 */
void queue_push(queue_t *q, tcp_conn_t *conn);

/**
 * @brief Pop the oldest tcp_conn_t from the queue
 * @param q Queue
 * @return tcp_conn_t* Dequeued connection, or NULL if empty
 */
tcp_conn_t* queue_pop(queue_t *q);

/**
 * @brief Return true if queue is empty
 * @param q Queue
 * @return true if empty, false otherwise
 */
bool queue_empty(queue_t *q);

/**
 * @brief Free all nodes in the queue and the queue itself
 * @param q Queue
 */
void queue_free(queue_t *q);

/**
 * @brief Peek at, but do not remove, the oldest pending connection in the queue.
 * @param q Queue
 * @return Pointer to the head @ref tcp_conn_t, or NULL if the queue is empty
 */
tcp_conn_t* queue_peek(queue_t *q);

/**
 * @brief Allocate a new file descriptor for a TCP connection.
 * @note O(n) time
 *
 * @param conn Pointer to the TCP connection.
 * @return File descriptor number on success, or -1 on failure.
 */
int tcp_allocate_fd(tcp_conn_t* conn);

/**
 * @brief Free a previously allocated TCP file descriptor.
 * @note O(1) time
 *
 * @param x File descriptor number to free.
 */
void tcp_free_fd(int x);

/**
 * @brief Find a TCP connection by its file descriptor.
 * @note O(1) time
 *
 * @param x File descriptor number.
 * @return Pointer to the TCP connection if found, or NULL if not found.
 */
tcp_conn_t* tcp_find_by_fd(int x);

/**
 * @brief Dump debug info for a TCP segment
 *
 * @param encap_packet Encapsulating IP packet
 * @param segment TCP segment
 * @param options TCP options
 * @param len TCP segment length (bytes)
 * @param our_checksum Calculated checksum
 * @param in True if inbound, false if outbound (direction tag)
 */
void tcp_dump_segment(bool in, tcp_conn_t* conn, const ip_packet_t* encap_packet, const tcp_segment_t* segment, const tcp_options_t* options, size_t len, uint16_t our_checksum);

/**
 * @brief Create a listening TCP socket.
 *
 * Binds a TCP connection control block (TCB) to the given local address and port, and transitions it into the LISTEN state.
 * A pending connection queue is created to hold half-open connections until they are accepted.
 *
 * @param addr Local IPv4 address to bind to (host byte order; 0 for all addresses)
 * @param port Local TCP port to listen on (host byte order)
 * @param backlog Maximum number of pending connections that may be queued
 * @return File descriptor for the listening socket on success, or a negative @ref tcp_error_code_t on failure
 */
int tcp_listen(uint32_t addr, uint16_t port, int backlog);

/**
 * @brief Accept an incoming connection on a listening socket (non-blocking).
 *
 * Returns a file descriptor for an already-ESTABLISHED pending connection at the head of the queue.
 * If no established connection is available, returns @ref TCP_ERROR_WOULD_BLOCK and leaves the queue unchanged.
 *
 * @param socket File descriptor of a socket previously placed in the LISTEN state using @ref tcp_listen()
 * @return File descriptor for the accepted connection on success, or a negative @ref tcp_error_code_t on failure
 */
int tcp_accept(int socket);

/**
 * @brief Internal non-blocking connect helper (active open).
 *
 * Creates a TCB, allocates/binds a local port, enters @ref TCP_SYN_SENT, and transmits a SYN.
 * The returned socket descriptor may be polled for establishment with @ref is_connected().
 *
 * @param target_addr Target IPv4 address (host byte order)
 * @param target_port Target TCP port (host byte order)
 * @param source_port Local source port to use, or 0 to choose automatically
 * @return Non-negative file descriptor on success; negative @ref tcp_error_code_t on failure
 */
int tcp_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port);

/**
 * @brief Return true if the socket’s transmit path is fully drained.
 *
 * Drained means: no application bytes buffered and no unacknowledged bytes in flight
 * (send buffer empty AND SND.UNA == SND.NXT). Non-blocking and lock-safe.
 *
 * @param fd Socket descriptor
 * @return true when fully drained, false otherwise
 */
bool sock_sent(int fd);