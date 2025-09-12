#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Process the SOCKWRITE statement in BASIC.
 *
 * The SOCKWRITE statement sends data to the specified socket. The data to be sent is provided
 * as a string variable.
 *
 * This function is asynchronous, it will add the data to the buffer for the socket and
 * return immediately, rather than waiting for confirmation of sending.
 *
 * @param ctx BASIC context.
 */
void sockwrite_statement(struct basic_ctx* ctx);

/**
 * @brief Perform a DNS lookup and return the corresponding IP address.
 *
 * The DNS$ function performs a DNS lookup for the specified hostname and returns the resolved IP address
 * as a string.
 *
 * @param ctx BASIC context.
 * @return The resolved IP address as a string.
 */
char* basic_dns(struct basic_ctx* ctx);

/**
 * @brief Get network information.
 *
 * The NETINFO$ function retrieves specific network-related information based on the provided string argument.
 * Possible values include "ip", "gw" (gateway), "mask" (netmask), and "dns".
 *
 * @param ctx BASIC context.
 * @return The requested network information as a string.
 */
char* basic_netinfo(struct basic_ctx* ctx);

/**
 * @brief Get the status of a socket.
 *
 * The SOCKSTATUS function returns the connection status of a socket. If the socket is connected, it returns 1;
 * otherwise, it returns 0.
 *
 * @param ctx BASIC context.
 * @return 1 if the socket is connected, 0 if not.
 */
int64_t basic_sockstatus(struct basic_ctx* ctx);

/**
 * @brief Read data from a socket in BASIC.
 *
 * The basic_insocket function checks if there is data available on the specified socket descriptor
 * and returns the data as a string. If no data is available, the function waits until it is ready.
 *
 * @param ctx BASIC context.
 * @return A string containing the data read from the socket.
 */
char* basic_insocket(struct basic_ctx* ctx);

/**
 * @brief Close a socket in BASIC.
 *
 * The SOCKCLOSE statement is used to close a network socket. The socket descriptor is specified,
 * and the connection is closed.
 *
 * @param ctx BASIC context.
 */
void sockclose_statement(struct basic_ctx* ctx);

/**
 * @brief Establish a network connection.
 *
 * The CONNECT statement is used to establish a network connection with a specified IP address
 * and port number. The socket descriptor is stored in the variable provided.
 *
 * @param ctx BASIC context.
 */
void connect_statement(struct basic_ctx* ctx);

/**
 * @brief Process the SOCKREAD statement in BASIC.
 *
 * The SOCKREAD statement yields while waiting for socket data. It repeatedly checks if
 * the socket has data available, and yields back to the OS if not ready. Once data is
 * available, the scheduler resumes the statement to read and store the value in a BASIC variable.
 *
 * @param ctx BASIC context.
 */
void sockread_statement(struct basic_ctx* ctx);

/**
 * @brief Process the UDPWRITE statement in BASIC.
 *
 * Sends a single UDP datagram to the specified destination address and port, optionally binding a source port.
 * Arguments are parsed from the BASIC statement and validated; on error a diagnostic is raised to the tokenizer.
 *
 * @param ctx BASIC context.
 */
void udpwrite_statement(struct basic_ctx* ctx);

/**
 * @brief Process the UDPBIND statement in BASIC.
 *
 * Binds a UDP “daemon” handler for the given local port so that inbound datagrams are queued for subsequent reads.
 * The handler stores packets in a per-port FIFO owned by the BASIC runtime.
 *
 * @param ctx BASIC context.
 */
void udpbind_statement(struct basic_ctx* ctx);

/**
 * @brief Process the UDPUNBIND statement in BASIC.
 *
 * Unregisters a previously bound UDP handler for the given local port and stops queueing packets for that port.
 *
 * @param ctx BASIC context.
 */
void udpunbind_statement(struct basic_ctx* ctx);

/**
 * @brief Read the next queued UDP packet payload for a port.
 *
 * Implements UDPREAD$ in BASIC. Pops the oldest queued datagram for the specified local port and returns its
 * payload as a newly allocated string. Side effects: updates the BASIC context’s “last packet” metadata
 * (source IP/port, length).
 *
 * @param ctx BASIC context.
 * @return Pointer to a NUL-terminated payload string (owned by the BASIC GC), or an empty string if no packet
 * is available.
 */
char* basic_udpread(struct basic_ctx* ctx);

/**
 * @brief Return the source port of the last UDP packet read.
 *
 * Exposes the metadata captured by the most recent successful UDPREAD$ call.
 *
 * @param ctx BASIC context.
 * @return Source UDP port number of the last packet, or 0 if no packet has been read.
 */
int64_t basic_udplastsourceport(struct basic_ctx* ctx);

/**
 * @brief Return the source IP address of the last UDP packet read.
 *
 * Exposes the metadata captured by the most recent successful UDPREAD$ call.
 *
 * @param ctx BASIC context.
 * @return Source IPv4 address as a dotted-quad string, or an empty string if no packet has been read.
 */
char* basic_udplastip(struct basic_ctx* ctx);

/**
 * @brief Create a listening TCP socket from BASIC.
 *
 * Implements SOCKLISTEN. Binds a TCP listener to the specified address and port with the requested backlog and
 * returns a file descriptor for subsequent SOCKACCEPT calls.
 *
 * @param ctx BASIC context.
 * @return Non-negative file descriptor on success; −1 with an error raised into the tokenizer on failure.
 */
int64_t basic_socklisten(struct basic_ctx* ctx);

/**
 * @brief Accept an incoming TCP connection from BASIC (non-blocking).
 *
 * Implements SOCKACCEPT. If an established connection is queued, returns a new file descriptor; if no
 * connection is currently ready, returns −1 without blocking. Errors are reported to the tokenizer.
 *
 * @param ctx BASIC context.
 * @return File descriptor of the accepted connection, or −1 if no connection is ready or on error.
 */
int64_t basic_sockaccept(struct basic_ctx* ctx);
