/**
 * @file devicename.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

#include "kernel.h"

/**
 * @brief Tracks allocation state for a device-name prefix.
 *
 * Each prefix (e.g. "hd", "cd", "ram") maintains a monotonically increasing
 * suffix counter used to generate globally unique device names. The owners
 * array maps suffix index -> human-readable subsystem name.
 */
typedef struct devname_prefix {
	/**
	 * @brief Device-name prefix string.
	 *
	 * Example values include "hd", "cd", "ram", "snd".
	 */
	const char* prefix;
	/**
	 * @brief Next suffix number to allocate.
	 *
	 * Device names are generated as prefix + incrementing integer.
	 */
	int increment;
	/**
	 * @brief Owner strings indexed by suffix id.
	 *
	 * owner[index] corresponds to prefix+index.
	 * Entries may be NULL if deregistered.
	 */
	const char **owners;
	/**
	 * @brief Number of allocated owner slots.
	 */
	size_t owner_count;
} devname_prefix_t;

/**
 * @brief Enumerated device-name entry.
 *
 * Returned by get_device_names() for diagnostic and listing purposes.
 */
typedef struct devname_entry {
	/**
	 * @brief Device-name prefix.
	 *
	 * Example: "hd"
	 */
	const char* name;
	/**
	 * @brief Numeric suffix component.
	 *
	 * Example: 0 for "hd0"
	 */
	int suffix;
	/**
	 * @brief Human-readable owner string.
	 *
	 * Example: "nvme", "ahci", "ramdisk".
	 */
	const char* owner;
} devname_entry_t;

/**
 * @brief Enumerate all currently registered device names.
 *
 * The caller supplies a fixed-size array which is populated with device-name
 * entries. If the supplied array is too small, the function returns false and
 * does not write beyond the provided bounds.
 *
 * @param entries Output array of device-name entries.
 * @param count Input: maximum number of entries available in @p entries.
 *              Output: number of entries written.
 * @return true on success, false on error or insufficient space.
 */
bool get_device_names(devname_entry_t *entries, size_t *count);

/**
 * @brief Remove a device-name registration.
 *
 * Deregisters a previously allocated suffix for a prefix. If all suffixes for
 * a prefix become unused, the prefix record itself is removed.
 *
 * @param prefix Device-name prefix.
 * @param suffix Numeric suffix to deregister.
 * @return true on success, false on error.
 */
bool deregister_device_name(const char* prefix, int suffix);

/**
 * @brief Allocate a globally unique device name.
 *
 * Generates a unique device name using the supplied prefix and writes the
 * resulting full name into @p buffer.
 *
 * Example:
 * - prefix "hd"
 * - result "hd0"
 *
 * The owner string is stored for diagnostic and enumeration purposes.
 *
 * @param prefix Device-name prefix.
 * @param owner Human-readable owner string.
 * @param buffer Output buffer receiving generated name.
 * @param buffer_len Size of @p buffer in bytes.
 * @return true on success, false on error or allocation failure.
 */
bool make_unique_device_name(const char *prefix, const char *owner, char *buffer, size_t buffer_len);

/**
 * @brief Get the number of active registered device names.
 *
 * Only entries with non-NULL owners are counted.
 *
 * @return Number of active device names.
 */
size_t get_device_name_count();

/**
 * @brief Initialise the device-name registry.
 *
 * Creates the internal hashmap used to track device-name prefixes and suffix
 * allocation state.
 */
void init_devicenames();