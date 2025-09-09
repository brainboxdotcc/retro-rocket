/**
 * @file tinyalloc.h
 * @brief Tinyalloc - a small, page-aware heap allocator for Retro Rocket.
 *
 * Provides aligned heap allocation and free operations with
 * low overhead. Intended for kernel use where predictable
 * allocation and fragmentation control are desirable.
 */

#pragma once

#include <kernel.h>

/**
 * Header structure for each allocation block in tinyalloc.
 *
 * Each allocated or free block in a managed region begins with this
 * header. The allocator uses it to track block size, free state, and
 * linked list ordering. User pointers returned by ta_alloc() point to
 * the memory immediately after this header.
 *
 * Fields:
 *  - size : Size of the usable payload in bytes (not including the header).
 *  - free : True if the block is available for allocation, false if in use.
 *  - next : Pointer to the next block header in address order.
 */
typedef struct ta_header {
	size_t size;		/**< Payload size in bytes (excludes header). */
	bool free;		/**< Allocation status flag. */
	struct ta_header *next;	/**< Next block header in the linked list. */
} ta_header;


/**
 * @brief Initialise the tinyalloc heap.
 *
 * Must be called once before using ta_alloc() or ta_free().
 *
 * @param heap      Pointer to the start of the heap region.
 * @param size      Total heap size in bytes.
 * @param alignment Minimum allocation alignment (power of two).
 */
void ta_init(void* heap, size_t size, size_t alignment);

/**
 * @brief Allocate a block of memory.
 *
 * The block is guaranteed to satisfy the alignment specified in ta_init().
 *
 * @param size Number of bytes requested.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void* ta_alloc(size_t size);

/**
 * @brief Free a previously allocated block.
 *
 * @param ptr Pointer returned by ta_alloc().
 * @return Size of the freed block in bytes, or 0 if the pointer was invalid.
 */
size_t ta_free(void* ptr);

/**
 * @brief Query the usable size of an allocated block.
 *
 * This may be equal to or larger than the requested size,
 * depending on allocator overhead and alignment.
 *
 * @param ptr Pointer returned by ta_alloc().
 * @return Usable block size in bytes.
 */
size_t ta_usable_size(void* ptr);

/**
 * Add an additional memory region to the allocator.
 *
 * This call registers a new, separate region of memory with the tinyalloc
 * allocator. The region is treated exactly like the initial heap space
 * passed to ta_init(), and allocations may come from any registered region.
 * Regions do not need to be contiguous with each other; gaps of unusable
 * memory between them are handled correctly. If two regions are physically
 * adjacent, they may be coalesced into a single larger free span when
 * blocks are freed.
 *
 * The first sizeof(ta_header) bytes of the supplied region are used for
 * allocator metadata. The caller must ensure the region is suitably aligned
 * and large enough to contain at least one header and some usable space.
 *
 * @param region  Pointer to the start of the new memory region.
 * @param size    Size of the region in bytes, including space for metadata.
 *
 * @return true if the region was added successfully, or false if the region
 *         is too small or invalid.
 */
bool ta_add_region(void *region, size_t size);