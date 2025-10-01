#pragma once
#include <stdint.h>
#include <stdbool.h>

enum cache_mode {
	cache_wb,   /* normal write-back (PWT=0, PCD=0) */
	cache_uc    /* uncacheable (PWT=1, PCD=1) – safe default for MMIO */
};

typedef uint64_t (*alloc_page_cb)(void);           /* returns physical address of a zeroed 4K page */
typedef void     (*free_page_cb)(uint64_t phys);   /* free a 4K page by physical address */

#define PTE_P         (1 << 0)
#define PTE_W         (1 << 1)
#define PTE_U         (1 << 2)
#define PTE_PWT       (1 << 3)
#define PTE_PCD       (1 << 4)
#define PTE_A         (1 << 5)
#define PTE_D         (1 << 6)
#define PTE_PS        (1 << 7)   /* For PDE (2M) / PDPTE (1G). For PTE this bit is PAT. */
#define PTE_G         (1 << 8)
#define PTE_NX        (1ull << 63)

#define PT_ENTRIES    512
#define PT_SHIFT      12
#define PD_SHIFT      21
#define PDP_SHIFT     30
#define PML4_SHIFT    39
#define PT_MASK       0x000ffffffffff000

bool map_range(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page);

bool map_identity(uint64_t pml4_phys, uint64_t base, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page);

bool unmap_range(uint64_t pml4_phys, uint64_t virt, uint64_t size);

void adopt_cloned_tables(void);

bool mmio_identity_map(uint64_t pml4_phys, uint64_t phys, uint64_t size);

bool ram_identity_map(uint64_t pml4_phys, uint64_t phys, uint64_t size, bool writable, bool executable);

bool identity_unmap(uint64_t pml4_phys, uint64_t phys, uint64_t size);
