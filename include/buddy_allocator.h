/**
 * @file buddy_allocator.h
 * @author Craig Edwards
 * @brief Growable multi-region buddy allocator for Retro Rocket subsystems.
 *
 * A buddy memory allocator that provides malloc/free-like semantics
 * on top of one or more contiguous backing pools. Each allocator
 * instance manages a linked list of regions, allowing it to grow
 * dynamically when out of memory. Regions are obtained from the
 * Retro Rocket base allocator (e.g. via kmalloc).
 *
 * Allocations are rounded up to the nearest power of two, with a
 * minimum block size set by @ref buddy_init. Freeing coalesces
 * buddy blocks within a region to minimise fragmentation.
 *
 * Complexity:
 * - Allocation: O(log n) within a region (splitting blocks).
 *               O(1) typical due to "active region" fast path.
 * - Free: O(1) using a region back-pointer in the block header.
 *
 * Notes:
 * - Not thread safe: wrap calls in a spinlock if used from multiple tasks.
 * - Each allocated block has overhead to store its order and region.
 * - Alignment is always a power of two, at least the minimum block size.
 * - Internal fragmentation can be significant for small allocations.
 * - Regions never merge with each other; fragmentation is managed per region.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>


/**
 * @brief Block header used for both free list nodes and allocated blocks.
 *
 * When free:
 *  - Only `next` is used (as a linked list pointer).
 *
 * When allocated:
 *  - `order` and `region` are valid metadata.
 *  - User data begins immediately after the header.
 */
typedef struct buddy_header {
	struct buddy_header *next;    /**< Next block in free list (valid when free) */
	int order;                    /**< Order of this block (valid when allocated) */
	struct buddy_region *region;  /**< Owning region (valid when allocated) */
} buddy_header_t;

/**
 * @brief Alias for clarity when manipulating free lists.
 */
typedef buddy_header_t buddy_block_t;

/**
 * @brief Buddy region structure (internal).
 *
 * Represents one contiguous pool of memory managed
 * by the buddy allocator. Multiple regions may exist.
 */
typedef struct buddy_region {
	uint8_t *pool;                       /**< Base pointer to backing memory pool */
	buddy_block_t *free_lists[32];       /**< Array of free lists, indexed by order */
	int min_order;                       /**< Minimum order (smallest block size) */
	int max_order;                       /**< Maximum order (largest block size) */
	struct buddy_region *next;           /**< Next region in linked list */
} buddy_region_t;

/**
 * @brief Growable buddy allocator instance.
 *
 * Each instance manages one or more buddy regions, adding new
 * regions as required when allocations exceed current capacity.
 */
typedef struct buddy_allocator {
	buddy_region_t *regions;       /**< Linked list of regions */
	buddy_region_t *active_region; /**< Current target region for allocations */
	int grow_order;                /**< Size of each new region in orders (1 << grow_order) */
	int min_order;                 /**< Minimum order (smallest block size) */
	int max_order;                 /**< Maximum order (largest block size) */
	size_t current_bytes;          /**< Current allocated bytes in this region */
	size_t peak_bytes;             /**< Peak allocated bytes in this region */
} buddy_allocator_t;

/**
 * @brief Initialise a growable buddy allocator.
 *
 * @param alloc       Allocator instance to initialise.
 * @param min_order   Minimum order (e.g. 6 for 64B blocks).
 * @param max_order   Maximum order (e.g. 22 for 4MB region).
 * @param grow_order  Order used when adding new regions (e.g. 22 for 4MB).
 *
 * The allocator starts with no regions. The first region is allocated
 * automatically on the first call to @ref buddy_malloc, or may be added
 * eagerly by growing manually.
 */
void buddy_init(buddy_allocator_t* alloc, int min_order, int max_order, int grow_order);

/**
 * @brief Allocate a block of memory from the buddy allocator.
 *
 * @param alloc  Allocator instance to allocate from.
 * @param size   Requested size in bytes.
 * @return Pointer to allocated memory, or NULL if out of memory.
 *
 * @note Alignment of the returned pointer is at least the largest power-of-two
 *       that divides sizeof(buddy_header_t); effectively 8 bytes with the current
 *       header layout. The underlying block base is aligned to its block size.
 *
 * If the active region cannot satisfy the request, a new region is
 * automatically allocated of size (1 << grow_order). Returned blocks
 * are aligned to their block size (power of two).
 */
void* buddy_malloc(buddy_allocator_t* alloc, size_t size);

/**
 * @brief Free a previously allocated block.
 *
 * @param alloc  Allocator instance.
 * @param ptr    Pointer returned by @ref buddy_malloc.
 *
 * The block is returned to its owning region’s free list. If its buddy
 * is also free, the two are merged into a larger block. The owning region
 * is identified via a back-pointer stored in the block header.
 */
void buddy_free(buddy_allocator_t* alloc, const void *ptr);

/**
 * @brief Destroy a buddy allocator and free all regions.
 *
 * @param alloc Allocator instance.
 *
 * All regions allocated internally via kmalloc() are freed
 * with kfree_null(). The allocator is left in an empty state.
 */
void buddy_destroy(buddy_allocator_t *alloc);

/**
 * @brief Duplicate a string into a buddy allocator heap.
 *
 * @param alloc  Buddy allocator to allocate from.
 * @param s      NUL-terminated string to duplicate.
 * @return Newly allocated string copy, or NULL on OOM.
 *
 * @note Alignment of the returned pointer is at least the largest power-of-two
 *       that divides sizeof(buddy_header_t); effectively 8 bytes with the current
 *       header layout. The underlying block base is aligned to its block size.
 *
 * The returned string is allocated from the buddy allocator's
 * private heap and must be freed with @ref buddy_free.
 */
char* buddy_strdup(buddy_allocator_t* alloc, const char *s);

/**
 * @brief Reallocate a block within a buddy allocator.
 *
 * Works like krealloc(): if ptr is NULL, behaves like buddy_malloc().
 * If size == 0, frees the block and returns NULL.
 *
 * @note Alignment of the returned pointer is at least the largest power-of-two
 *       that divides sizeof(buddy_header_t); effectively 8 bytes with the current
 *       header layout. The underlying block base is aligned to its block size.
 *
 * @param alloc  Buddy allocator to allocate from.
 * @param ptr    Pointer previously returned by buddy_malloc().
 * @param size   New desired size in bytes.
 * @return       Pointer to resized block, or NULL on OOM.
 */
void* buddy_realloc(buddy_allocator_t* alloc, void *ptr, size_t size);

/**
 * @brief Allocate zero-initialised memory from the buddy allocator.
 *
 * Behaves like standard calloc(): allocates space for @p num elements of
 * @p size bytes each, rounds up to the allocator’s block size, and fills
 * the returned block with zeroes.
 *
 * @param alloc Buddy allocator to allocate from.
 * @param num   Number of elements to allocate.
 * @param size  Size of each element in bytes.
 * @return      Pointer to zero-initialised memory, or NULL on OOM.
 *
 * @note Alignment of the returned pointer is at least the largest power-of-two
 *       that divides sizeof(buddy_header_t); effectively 8 bytes with the current
 *       header layout. The underlying block base is aligned to its block size.
 *
 * @see buddy_malloc(), buddy_free(), buddy_realloc(), buddy_destroy()
 */
void* buddy_calloc(buddy_allocator_t* alloc, size_t num, size_t size);

/**
 * Ensure that if we make changes here, pointers returned by the buddy allocator
 * are still aligned to at least 8 bytes, as some systems assume this.
 */
_Static_assert((sizeof(buddy_header_t) % 8) == 0, "Alignment too small");