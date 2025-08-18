/**
 * @file block_cache.h
 * @brief Per-device write-through block cache.
 *
 * This cache sits above block device drivers. It caches fixed-size sectors,
 * evicting the oldest (by last access) when full. Reads and writes are
 * write-through and counted as accesses. One cache instance is created
 * per storage device.
 */
#pragma once
#include <kernel.h>

/* Forward declarations */
typedef struct block_cache block_cache_t;

/**
 * @brief Create a new cache for a storage device.
 *
 * @param dev  Device to attach cache to.
 * @return     Pointer to cache instance, or NULL on error.
 */
block_cache_t *block_cache_create(storage_device_t *dev);

/**
 * @brief Destroy a cache and free all associated resources.
 *
 * @param pcache  Pointer to cache pointer; set to NULL on return.
 */
void block_cache_destroy(block_cache_t **pcache);

/**
 * @brief Read bytes from a cached device.
 *
 * Granularity is per sector; the cache works in multiples of the device’s
 * block size. Reads hitting the cache are served immediately; misses are
 * read from the device and then cached.
 *
 * @param c      Cache instance.
 * @param lba    Starting sector number.
 * @param bytes  Number of bytes to read.
 * @param out    Destination buffer.
 * @return       1 on success, 0 on failure (fs_set_error() is set).
 */
int block_cache_read(block_cache_t *c, uint64_t lba, uint32_t bytes, unsigned char *out);

/**
 * @brief Write bytes through to a cached device.
 *
 * Writes are always passed to the device. The cache is updated (write-allocate)
 * so subsequent reads will hit.
 *
 * @param c      Cache instance.
 * @param lba    Starting sector number.
 * @param bytes  Number of bytes to write.
 * @param src    Source buffer.
 * @return       1 on success, 0 on failure (fs_set_error() is set).
 */
int block_cache_write(block_cache_t *c, uint64_t lba, uint32_t bytes, const unsigned char *src);

/**
 * @brief Invalidate all entries in a cache.
 *
 * Useful when a device is reset or forcibly changed.
 *
 * @param c  Cache instance.
 */
void block_cache_invalidate(block_cache_t *c);

