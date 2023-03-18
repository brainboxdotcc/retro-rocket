#pragma once

#include "kernel.h"

/**
 * @brief Host to network byte
 * 
 * @param byte input
 * @param num_bits number of bits of high nibble
 * @return uint8_t network byte order
 */
uint8_t htonb(uint8_t byte, int num_bits);

/**
 * @brief Network to host byte
 * 
 * @param byte input
 * @param num_bits number of bits of high nibble
 * @return uint8_t host byte order
 */
uint8_t ntohb(uint8_t byte, int num_bits);

/**
 * @brief Host to network short
 * 
 * @param hostshort input
 * @return uint16_t network order
 */
uint16_t htons(uint16_t hostshort);

/**
 * @brief Host to network long
 * 
 * @param hostlong input
 * @return uint32_t network order
 */
uint32_t htonl(uint32_t hostlong);

/**
 * @brief Network to host short
 * 
 * @param netshort input
 * @return uint16_t host order
 */
uint16_t ntohs(uint16_t netshort);

/**
 * @brief Network to host long
 * 
 * @param netlong input
 * @return uint32_t host order
 */
uint32_t ntohl(uint32_t netlong);
