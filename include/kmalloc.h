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
 * @brief Initialises the kernel’s dynamic memory and DMA-friendly “low heap”.
 *
 * @details
 * `init_heap()` brings up two complementary allocation domains and reconciles the
 * bootloader’s memory hand-off into something the kernel can safely and efficiently
 * allocate from:
 *
 * 1) **Low heap (fixed, DMA-friendly window).**
 *    - Automatically locates a *contiguous 12 MB* region of **USABLE** RAM below 4 GiB
 *      and assigns it to `LOW_HEAP_START` / `LOW_HEAP_MAX` (both `uint32_t`).
 *    - This window is reserved exclusively for the bump allocator used by
 *      `kmalloc_low()` and returns **32-bit physical addresses** suitable for legacy
 *      devices and DMA engines that cannot address high memory.
 *    - The low-heap window is deliberately **excluded** from the general allocator to
 *      avoid fragmentation and to guarantee that drivers always have the expected
 *      12 MB of low physical memory available.
 *
 * 2) **General heap (primary + additional regions).**
 *    - Chooses the *largest* **USABLE** memmap entry as the primary arena and
 *      initialises the general allocator (`allocator_init`) over it, **carving out** the
 *      low-heap window if they overlap.
 *    - Iterates over all remaining **USABLE** entries and adds them with
 *      `allocator_add_region`, again **clipping out** the low-heap window so it is never
 *      accidentally pooled into the general heap.
 *
 * 3) **Selective reclamation of bootloader allocations.**
 *    - Walks all **Bootloader Reclaimable** (BLR) entries and returns them to the
 *      general heap *except* when either of the following is true:
 *        • the region is “small” (currently < 1 MB), which typically holds loader
 *          scaffolding such as page tables, descriptor tables, or tiny request
 *          structs; or
 *        • the region contains any *live* pointer supplied by the bootloader
 *          (e.g., Limine response structures, GDT base, CR3), in which case the
 *          whole BLR span is kept.
 *    - This yields the large BLR “scratch” arenas while conservatively preserving
 *      small or still-referenced chunks. The result is that almost all of the
 *      bootloader’s memory footprint becomes usable to the kernel immediately.
 *
 * **What is *not* added to the general heap**
 *  - **Kernel+Modules** regions (the kernel image and loaded modules).
 *  - The **Framebuffer** region.
 *  - Any **ACPI NVS**, **ACPI Reclaimable**, **Bad Memory** or **Reserved** ranges.
 *
 * **Why this design?**
 *  - The low heap provides a guaranteed, contiguous, sub-4 GB pool for drivers and
 *    DMA without requiring the kernel to juggle bounce buffers or IOMMU mappings.
 *  - Carving the low heap out once, and everywhere, prevents subtle re-introduction
 *    into the general allocator.
 *  - Conservative BLR reclamation gives you back the big win (hundreds of MB on
 *    typical firmware) with minimal complexity, while avoiding the risk of freeing
 *    loader memory that is still referenced during early boot.
 *
 * Should be called **once**, early in kernel initialisation, before any dynamic
 * memory use by subsystems or drivers.
 */
void init_heap(void);

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

void* kmalloc_aligned(uint64_t size, uint64_t align);

void kfree_aligned(const void* ptr);

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

