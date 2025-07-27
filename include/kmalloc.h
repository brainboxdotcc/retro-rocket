/**
 * @file kmalloc.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
/**
 * @file kmalloc.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <stddef.h>

/**
 * @brief Initialise the kernel heap.
 *
 * Sets up internal data structures required for dynamic memory allocation.
 * Should be called early during kernel initialisation.
 */
void init_heap(void);

/**
 * @brief Print diagnostic heap information to the terminal.
 *
 * Outputs allocator statistics such as used, free, and total heap space.
 * Primarily for debugging and diagnostics.
 */
void print_heapinfo(void);

/**
 * @brief Allocate memory from the kernel heap.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void* kmalloc(uint64_t size);

/**
 * @brief Free previously allocated memory.
 *
 * @param addr Pointer previously returned by kmalloc/kcalloc/krealloc.
 */
void kfree(const void* addr);

/**
 * @brief Allocate zero-initialised memory.
 *
 * @param num Number of elements.
 * @param size Size of each element in bytes.
 * @return Pointer to allocated memory, or NULL if allocation fails.
 */
void* kcalloc(size_t num, size_t size);

/**
 * @brief Resize previously allocated memory block.
 *
 * Preserves contents up to the lesser of old and new sizes.
 *
 * @param ptr Pointer to memory allocated by kmalloc/kcalloc/krealloc.
 * @param new_size New size in bytes.
 * @return Pointer to reallocated memory, or NULL if resizing fails.
 */
void* krealloc(void* ptr, size_t new_size);

/**
 * @brief Allocate memory from low (identity-mapped) memory under 4GB.
 *
 * Intended for DMA buffers or page tables that require low physical addresses.
 *
 * @param size Number of bytes to allocate.
 * @return Physical address of the allocated block (below 4GB).
 */
uint32_t kmalloc_low(uint32_t size);

/**
 * @brief Free a low-memory allocation made with kmalloc_low().
 *
 * Only needed if low memory is explicitly managed and reused.
 *
 * @param addr Physical address of the block to free.
 */
void kfree_low(uint32_t addr);

/**
 * @brief Get total amount of free memory.
 *
 * @return Free heap space in bytes.
 */
uint64_t get_free_memory(void);

/**
 * @brief Get total amount of used memory.
 *
 * @return Used heap space in bytes.
 */
uint64_t get_used_memory(void);

/**
 * @brief Get total available memory managed by the kernel heap.
 *
 * @return Total heap size in bytes.
 */
uint64_t get_total_memory(void);

/**
 * @brief Halt the system with a preboot failure message.
 *
 * Used when a critical failure occurs before full initialisation.
 *
 * @param msg Null-terminated string describing the failure.
 */
void preboot_fail(const char* msg);

/**
 * @brief Free memory and set the pointer to NULL.
 *
 * This macro safely frees a dynamically allocated pointer and sets it to NULL
 * to avoid dangling references. It must be passed the address of the pointer
 * (i.e., a pointer to the pointer).
 *
 * Example usage:
 * @code
 * void* ptr = kmalloc(128);
 * kfree_null(&ptr); // ptr is now NULL
 * @endcode
 *
 * @param p A pointer to the pointer to be freed. Must not be NULL.
 */
#define kfree_null(p) do { kfree(*(p)); *(p) = NULL; } while (0)

