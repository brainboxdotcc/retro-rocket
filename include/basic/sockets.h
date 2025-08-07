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
