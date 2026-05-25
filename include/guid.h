#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Length of a GUID in ASCII (including dashes and null terminator).
 */
#define GUID_ASCII_LEN 37

/**
 * @brief Length of a GUID in binary form.
 */
#define GUID_BINARY_LEN 16

/**
 * Human-readable GUID string representation. Null terminated.
 */
typedef char text_guid_t[GUID_ASCII_LEN];

/**
 * Raw on-disk GUID byte layout
 */
typedef uint8_t binary_guid_t[GUID_BINARY_LEN];

/**
 * @brief Convert a GUID from ASCII to binary format.
 *
 * @param guid ASCII string representation of the GUID.
 * @param binary Output buffer for binary GUID (16 bytes).
 * @return true if conversion succeeded, false on error.
 */
bool guid_to_binary(const text_guid_t guid, binary_guid_t binary);

/**
 * @brief Convert a binary GUID to ASCII format.
 *
 * @param binary Binary GUID (16 bytes).
 * @param guid Output buffer for ASCII GUID (37 bytes including null terminator).
 * @return true if conversion succeeded, false on error.
 */
bool binary_to_guid(const binary_guid_t binary, text_guid_t guid);

/**
 * Compare two binary GUIDs for equality.
 *
 * @param a First GUID
 * @param b Second GUID
 * @return true if the GUIDs match
 */
bool match_guid_binary(const binary_guid_t a, const binary_guid_t b);

/**
 * Compare two textual GUIDs for equality.
 *
 * Comparison is case-insensitive.
 *
 * @param a First GUID
 * @param b Second GUID
 * @return true if the GUIDs match
 */
bool match_guid_text(const text_guid_t a, const text_guid_t b);