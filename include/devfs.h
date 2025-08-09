/**
 * @file devfs.h
 * @author Craig Edwards
 * @brief Device filesystem interface for Retro Rocket.
 *
 * The Device Filesystem (devfs) is a virtual filesystem that exposes
 * read-only text-based device nodes to userland (e.g. BASIC programs)
 * under the `/devices` path.
 *
 * Each registered device node appears as a file in `/devices` and is backed
 * by a pair of callbacks:
 *  - An update callback, invoked periodically, to refresh the file's metadata
 *    (e.g., size or flags) before it is read.
 *  - A read callback, invoked when userland code reads from the device file,
 *    to copy data into the provided buffer.
 *
 * Only text devices are currently supported. Write operations are not supported.
 *
 * @copyright Copyright (c) 2012–2025
 */
#pragma once
#include <kernel.h>

/**
 * @brief Device node periodic update callback type.
 *
 * This callback is invoked by the devfs idle task. Implementations can
 * update @p ent->size, @p ent->flags, or other metadata fields.
 *
 * @param ent Pointer to the directory entry structure for this device node.
 */
typedef void (*devfs_update_cb_t)(struct fs_directory_entry_t *ent);

/**
 * @brief Device node read callback type.
 *
 * This callback is invoked when userland reads from the device file.
 * Implementations should copy exactly @p length bytes from the device's
 * current state into @p buffer, starting at the given @p start offset.
 *
 * @param start  Offset in bytes from which to begin reading.
 * @param length Number of bytes to read.
 * @param buffer Destination buffer to receive the read data.
 *
 * @return true on success, false on failure (set filesystem error code with fs_set_error()).
 */
typedef bool (*devfs_read_cb_t)(uint64_t start, uint32_t length, unsigned char *buffer);

/**
 * @brief Linked list node representing a registered devfs entry.
 *
 * This structure is used internally by devfs to store each registered
 * device node along with its associated callbacks.
 */
typedef struct devfs_node {
	fs_directory_entry_t ent;    /**< Directory entry metadata for the device. */
	devfs_update_cb_t update_cb; /**< Periodic update callback. */
	devfs_read_cb_t read_cb;     /**< Read callback. */
	struct devfs_node *next;     /**< Pointer to the next node in the list. */
} devfs_node_t;

/**
 * @brief Initialise the device filesystem.
 *
 * Registers the devfs with the VFS layer under `/devices`. Must be called
 * during system initialisation before registering any device nodes.
 */
void init_devfs(void);

/**
 * @brief Register a new text device file in devfs.
 *
 * @param name      Name of the device file (e.g., "debug").
 * @param update_cb Periodic update callback (may be NULL if not needed).
 * @param read_cb   Read callback (must not be NULL).
 *
 * The file will appear under `/devices/<name>`. Names should not contain
 * path separators or exceed the maximum filename length supported by the VFS.
 */
void devfs_register_text(const char *name, devfs_update_cb_t update_cb, devfs_read_cb_t read_cb);

/**
 * @brief Register the built-in `/devices/debug` log device.
 *
 * This device provides a scrollback buffer for dprintf() output, making
 * kernel debug logs accessible from userland (e.g. via BASIC OPENUP/INPUT).
 *
 * Should be called after init_devfs().
 */
void init_debuglog(void);
