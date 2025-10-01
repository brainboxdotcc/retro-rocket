#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @enum cache_mode
 * Page cache attributes for new mappings.
 */
enum cache_mode {
	cache_wb,   /**< Write-back: normal cached memory (PWT=0, PCD=0) */
	cache_uc    /**< Uncacheable: safe default for MMIO (PWT=1, PCD=1) */
};

typedef struct gdt_t {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_t;

/**
 * @typedef alloc_page_cb
 * Callback to allocate a zeroed 4 KiB page, returning its physical address
 */
typedef uint64_t (*alloc_page_cb)(void);

/**
 * @typedef free_page_cb
 * Callback to free a 4 KiB page by physical address
 */
typedef void (*free_page_cb)(uint64_t phys);

/* Page table entry flags */
#define PTE_PRESENT		(1 << 0)   /**< Present */
#define PTE_WRITE		(1 << 1)   /**< Writable */
#define PTE_USER		(1 << 2)   /**< User accessible */
#define PTE_WRITE_THROUGH	(1 << 3)   /**< Write-through caching */
#define PTE_CACHE_DISABLED	(1 << 4)   /**< Cache disable */
#define PTE_ACCESSED		(1 << 5)   /**< Accessed */
#define PTE_DIRTY		(1 << 6)   /**< Dirty (for PTE) */
#define PTE_PAGE_SIZE		(1 << 7)   /**< Page size: 2 MB (PD) / 1 GB (PDP). For PTE this bit is PAT */
#define PTE_GLOBAL		(1 << 8)   /**< Global mapping */
#define PTE_NO_EXECUTE		(1ull << 63) /**< No-execute */

/* Page table constants */
#define PT_ENTRIES	512
#define PT_SHIFT	12
#define PD_SHIFT	21
#define PDP_SHIFT	30
#define PML4_SHIFT	39
#define PT_MASK		0x000ffffffffff000

/**
 * Map a physical range to a specified virtual range
 *
 * @param virt      Starting virtual address (4 KiB aligned)
 * @param phys      Starting physical address (4 KiB aligned)
 * @param size      Mapping size in bytes (multiple of 4 KiB)
 * @param writable  If true, mapping is writable.
 * @param executable If true, mapping is executable; otherwise NX is set
 * @param cmode     Cache attribute for the mapping
 * @param alloc_page Allocator for new page-table pages
 * @return true if the range was mapped successfully, false otherwise
 */
bool map_range(uint64_t virt, uint64_t phys, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page);

/**
 * Identity-map a physical range
 *
 * @param base      Starting physical address (used as VA and PA)
 * @param size      Mapping size in bytes (multiple of 4 KiB)
 * @param writable  If true, mapping is writable
 * @param executable If true, mapping is executable; otherwise NX is set
 * @param cmode     Cache attribute for the mapping
 * @param alloc_page Allocator for new page-table pages
 * @return true if the range was mapped successfully, false otherwise
 */
bool map_identity(uint64_t base, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page);

/**
 * Unmap a virtual range
 *
 * @param virt      Starting virtual address (4K aligned)
 * @param size      Range size in bytes (multiple of 4K)
 * @return true if the range was unmapped successfully, false otherwise
 */
bool unmap_range(uint64_t virt, uint64_t size);

/**
 * Clone Limine’s initial page tables, switch CR3 to the new copy,
 * and preserve all existing mappings (identity and HHDM)
 * Must be called once, very early in boot, before interrupts are enabled
 */
void adopt_cloned_tables(void);

/**
 * Identity-map a physical MMIO region with UC attributes
 *
 * @param phys      Physical base address (4 KiB aligned)
 * @param size      Mapping size in bytes (multiple of 4K)
 * @return true if the range was mapped successfully, false otherwise
 */
bool mmio_identity_map(uint64_t phys, uint64_t size);

/**
 * Identity-map a physical RAM region with WB attributes
 *
 * @param phys      Physical base address (4 KiB aligned)
 * @param size      Mapping size in bytes (multiple of 4K)
 * @param writable  If true, mapping is writable
 * @param executable If true, mapping is executable; otherwise NX is set
 * @return true if the range was mapped successfully, false otherwise
 */
bool ram_identity_map(uint64_t phys, uint64_t size, bool writable, bool executable);

/**
 * Unmap a previously identity-mapped physical range
 *
 * @param phys      Physical base address (4 KiB aligned)
 * @param size      Mapping size in bytes (multiple of 4K)
 * @return true if the range was unmapped successfully, false otherwise
 */
bool identity_unmap(uint64_t phys, uint64_t size);

/**
 * Adopt the already-prepared page tables on an AP
 * Must have run adopt_cloned_tables() on the BSP first
 * Panics if interrupts are enabled, safe to call multiple times
 */
void adopt_cloned_tables_on_ap(void);

/**
 * Invalidate page cache
 * @param va virtual address
 */
static inline __attribute((always_inline)) void invlpg(void *va) {
	__asm__ volatile ("invlpg (%0)" :: "r"(va) : "memory");
}