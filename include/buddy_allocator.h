/**
 * @file buddy.h
 * @author Craig Edwards
 * @brief Multi-instance buddy allocator for Retro Rocket subsystems.
 *
 * A buddy memory allocator that provides malloc/free-like semantics
 * on top of a fixed backing pool. Each allocator instance manages
 * its own region, allowing safe disposal of the pool via kfree_null
 * when the instance is no longer needed.
 *
 * Allocations are rounded up to the nearest power of two, with a
 * minimum block size set by @ref buddy_init. Freeing coalesces
 * buddy blocks to minimise external fragmentation.
 *
 * Complexity:
 * - Allocation: O(log n) worst case (splitting blocks).
 * - Free: O(1) (merge with buddy if possible).
 *
 * Notes:
 * - Not thread safe: wrap calls in a spinlock if used from multiple tasks.
 * - Each allocated block has 4 bytes of overhead to store its order.
 * - Alignment is always a power of two, at least the minimum block size.
 * - Internal fragmentation can be significant for small allocations.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Free list node structure (internal use).
 */
typedef struct buddy_block {
	struct buddy_block *next; /**< Next block in free list */
} buddy_block_t;

/**
 * @brief Buddy allocator instance.
 *
 * Each instance manages one contiguous memory region obtained
 * from the Retro Rocket base allocator (e.g. via kmalloc).
 * Dispose of the entire instance by calling kfree_null() on
 * the @ref pool pointer after teardown.
 */
typedef struct buddy_allocator {
	uint8_t *pool;                  /**< Base pointer to backing memory pool */
	buddy_block_t *free_lists[32];  /**< Array of free lists, indexed by order */
	int max_order;                  /**< Maximum order (largest block size) */
	int min_order;                  /**< Minimum order (smallest block size) */
	size_t current_bytes;           /**< Currently allocated bytes */
	size_t peak_bytes;              /**< Peak allocated bytes */
} buddy_allocator_t;


/**
 * @brief Initialise a buddy allocator on a given memory region.
 *
 * @param alloc       Allocator instance to initialise.
 * @param backing     Pointer to backing pool memory.
 * @param min_order   Minimum order (e.g. 12 for 4KB blocks).
 * @param max_order   Maximum order (e.g. 20 for 1MB pool).
 *
 * The backing region must be at least (1 << max_order) bytes
 * and aligned to that size.
 */
void buddy_init(buddy_allocator_t *alloc, void *backing, int min_order, int max_order);

/**
 * @brief Allocate a block of memory from the buddy allocator.
 *
 * @param alloc  Allocator instance to allocate from.
 * @param size   Requested size in bytes.
 * @return Pointer to allocated memory, or NULL if out of memory.
 *
 * Returned blocks are at least @ref min_order in size, and
 * aligned to their block size (power of two).
 */
void *buddy_malloc(buddy_allocator_t *alloc, size_t size);

/**
 * @brief Free a previously allocated block.
 *
 * @param alloc  Allocator instance.
 * @param ptr    Pointer returned by @ref buddy_malloc.
 *
 * The block is returned to the free list, and if its buddy
 * is also free, the two are merged into a larger block.
 */
void buddy_free(buddy_allocator_t *alloc, void *ptr);

