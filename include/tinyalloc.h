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
 * @brief Initialise the tinyalloc heap.
 *
 * Must be called once before using ta_alloc() or ta_free().
 *
 * @param heap      Pointer to the start of the heap region.
 * @param size      Total heap size in bytes.
 * @param alignment Minimum allocation alignment (power of two).
 * @param pagesize  Size of an allocation page; used for splitting.
 */
void ta_init(void* heap, size_t size, size_t alignment, size_t pagesize);

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
