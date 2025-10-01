#include <kernel.h>

static uint64_t limine_gdtr;
static uint64_t limine_cr3;
static uint64_t new_cr3;
gdt_t gdtr;

static uint64_t pmm_alloc_page(void);

volatile struct limine_stack_size_request stack_size_request = {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = KSTACK_SIZE,
};

volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0,
};

volatile struct limine_kernel_address_request address_request = {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.revision = 0,
};

extern volatile struct limine_smp_request smp_request;
extern volatile struct limine_rsdp_request rsdp_request;
extern volatile struct limine_module_request module_request;
extern volatile struct limine_memmap_request memory_map_request;
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_kernel_file_request rr_kfile_req;

struct reqset request_addresses(void) {
	static struct {
		uintptr_t arr[12];
	} buf; /* backing storage, static so it survives return */

	buf.arr[0]  = (uintptr_t)limine_gdtr;
	buf.arr[1]  = (uintptr_t)limine_cr3;
	buf.arr[2]  = (uintptr_t)smp_request.response - hhdm_request.response->offset;
	buf.arr[3]  = (uintptr_t)rsdp_request.response - hhdm_request.response->offset;
	buf.arr[4]  = (uintptr_t)module_request.response - hhdm_request.response->offset;
	buf.arr[5]  = (uintptr_t)memory_map_request.response - hhdm_request.response->offset;
	buf.arr[6]  = (uintptr_t)framebuffer_request.response - hhdm_request.response->offset;
	buf.arr[7]  = (uintptr_t)rr_kfile_req.response - hhdm_request.response->offset;
	buf.arr[8]  = (uintptr_t)address_request.response - hhdm_request.response->offset;
	buf.arr[9]  = (uintptr_t)hhdm_request.response - hhdm_request.response->offset;
	buf.arr[10] = (uintptr_t)stack_size_request.response - hhdm_request.response->offset;

	uintptr_t rsp_va;
	__asm__ volatile ("mov %%rsp, %0" : "=r"(rsp_va));
	buf.arr[11] = rsp_va - 16 - (uintptr_t)hhdm_request.response->offset;

	return (struct reqset){
		buf.arr,
		sizeof(buf.arr) / sizeof(buf.arr[0])
	};
}

static uint64_t clone_table_recursive(uint64_t table_phys, int level, alloc_page_cb alloc_page) {
	uint64_t *src = (uint64_t *)table_phys;

	uint64_t new_phys = alloc_page();
	uint64_t *dst = (uint64_t *)new_phys;
	memset(dst, 0, 4096);

	for (int i = 0; i < PT_ENTRIES; i++) {
		uint64_t e = src[i];
		if ((e & PTE_PRESENT) == 0) {
			continue;
		}

		if (level < 3 && (e & PTE_PAGE_SIZE) == 0) {
			/* Non-leaf: allocate and clone child */
			uint64_t child_phys = e & PT_MASK;
			uint64_t cloned_child = clone_table_recursive(child_phys, level + 1, alloc_page);
			uint64_t flags = e & ~PT_MASK;
			dst[i] = (cloned_child & PT_MASK) | flags;
		} else {
			/* Leaf (4K) or large page (2M/1G): copy entry as-is */
			dst[i] = e;
		}
	}

	return new_phys;
}

/* Public entry: clone current CR3 hierarchy and return new CR3 physical */
uint64_t pagetables_clone_deep(alloc_page_cb alloc_page) {
	uint64_t cr3_phys;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3_phys));
	return clone_table_recursive(cr3_phys, 0, alloc_page);
}

/* Switch to a given PML4 physical page */
void pagetables_switch(uint64_t new_cr3_phys) {
	__asm__ volatile ("mov %0, %%cr3" :: "r"(new_cr3_phys) : "memory");
}

static void dump_mapping(uint64_t virt, uint64_t phys, uint64_t flags, int level) {
	const char *sizes[] = { "4K", "2M", "1G" };
	if (3 - level == 2) {
		dprintf("VA 0x%lx -> PA 0x%lx [%s%s%s%s] (%s)\n",
			virt, phys,
			flags & PTE_PRESENT ? "P" : "-",
			flags & PTE_WRITE ? "W" : "-",
			flags & PTE_NO_EXECUTE ? "NX" : "-",
			flags & PTE_PAGE_SIZE ? "PS" : "--",
			sizes[3 - level]);
	}
}

static uint64_t ensure_pt_for_va(uint64_t pml4_phys, uint64_t va, alloc_page_cb alloc_page) {
	int idx_pml4 = (int)((va >> PML4_SHIFT) & 511);
	int idx_pdp  = (int)((va >> PDP_SHIFT)  & 511);
	int idx_pd   = (int)((va >> PD_SHIFT)   & 511);
	int idx_pt   = (int)((va >> PT_SHIFT)   & 511);

	uint64_t *pml4 = (uint64_t *)pml4_phys;

	/* PDP level */
	if ((pml4[idx_pml4] & PTE_PRESENT) == 0) {
		uint64_t new_phys = alloc_page();
		uint64_t *zero = (uint64_t *)new_phys;
		memset(zero, 0, 4096);
		pml4[idx_pml4] = (new_phys & PT_MASK) | PTE_PRESENT | PTE_WRITE;
	}

	uint64_t pdpt_phys = pml4[idx_pml4] & PT_MASK;
	uint64_t *pdpt = (uint64_t *)pdpt_phys;

	/* PD level */
	if ((pdpt[idx_pdp] & PTE_PRESENT) == 0) {
		uint64_t new_phys = alloc_page();
		uint64_t *zero = (uint64_t *)new_phys;
		memset(zero, 0, 4096);
		pdpt[idx_pdp] = (new_phys & PT_MASK) | PTE_PRESENT | PTE_WRITE;
	} else if (pdpt[idx_pdp] & PTE_PAGE_SIZE) {
		/* Do not break an existing 1G mapping here */
		return 0;
	}

	uint64_t pd_phys = pdpt[idx_pdp] & PT_MASK;
	uint64_t *pd = (uint64_t *)pd_phys;

	/* PT level */
	if ((pd[idx_pd] & PTE_PRESENT) == 0) {
		uint64_t new_phys = alloc_page();
		uint64_t *zero = (uint64_t *)new_phys;
		memset(zero, 0, 4096);
		pd[idx_pd] = (new_phys & PT_MASK) | PTE_PRESENT | PTE_WRITE;
	} else if (pd[idx_pd] & PTE_PAGE_SIZE) {
		/* Do not break an existing 2M mapping here */
		return 0;
	}

	uint64_t pt_phys = pd[idx_pd] & PT_MASK;
	(void)idx_pt;
	return pt_phys;
}

static inline uint64_t cache_bits(enum cache_mode mode) {
	if (mode == cache_uc) {
		return PTE_WRITE_THROUGH | PTE_CACHE_DISABLED;
	}
	return 0;
}

bool map_range(uint64_t virt, uint64_t phys, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page) {
	if ((virt & 0xFFF) != 0 || (phys & 0xFFF) != 0 || size == 0) {
		return false;
	}

	uint64_t flags = PTE_PRESENT | (writable ? PTE_WRITE : 0) | (executable ? 0 : PTE_NO_EXECUTE) | cache_bits(cmode);

	uint64_t end = virt + size;
	while (virt < end) {
		uint64_t pt_phys = ensure_pt_for_va(new_cr3, virt, alloc_page);
		if (pt_phys == 0) {
			/* Refuse to split existing large pages in this helper */
			return false;
		}

		uint64_t *pt = (uint64_t *)pt_phys;
		int idx_pt = (int)((virt >> PT_SHIFT) & 511);
		pt[idx_pt] = (phys & PT_MASK) | flags;
		invlpg((void *)virt);

		virt += 4096;
		phys += 4096;
	}

	return true;
}

/* Convenience identity map */
bool map_identity(uint64_t base, uint64_t size, bool writable, bool executable, enum cache_mode cmode, alloc_page_cb alloc_page) {
	return map_range(base, base, size, writable, executable, cmode, alloc_page);
}

/* Unmap [virt .. virt+size), 4K-granular. Does not tear down empty tables */
bool unmap_range(uint64_t virt, uint64_t size) {
	if ((virt & 0xFFF) != 0 || size == 0) {
		return false;
	}

	uint64_t end = virt + size;
	while (virt < end) {
		int idx_pml4 = (int)((virt >> PML4_SHIFT) & 511);
		int idx_pdp  = (int)((virt >> PDP_SHIFT)  & 511);
		int idx_pd   = (int)((virt >> PD_SHIFT)   & 511);
		int idx_pt   = (int)((virt >> PT_SHIFT)   & 511);

		uint64_t *pml4 = (uint64_t *)new_cr3;
		if ((pml4[idx_pml4] & PTE_PRESENT) == 0) {
			return false;
		}
		uint64_t *pdpt = (uint64_t *)(pml4[idx_pml4] & PT_MASK);
		if ((pdpt[idx_pdp] & PTE_PRESENT) == 0 || (pdpt[idx_pdp] & PTE_PAGE_SIZE) != 0) {
			return false;
		}
		uint64_t *pd = (uint64_t *)(pdpt[idx_pdp] & PT_MASK);
		if ((pd[idx_pd] & PTE_PRESENT) == 0 || (pd[idx_pd] & PTE_PAGE_SIZE) != 0) {
			return false;
		}
		uint64_t *pt = (uint64_t *)(pd[idx_pd] & PT_MASK);

		pt[idx_pt] = 0;
		invlpg((void *)virt);

		virt += 4096;
	}

	return true;
}

/* Panics if the heap can’t supply a 4 KiB page */
static uint64_t pmm_alloc_page(void) {
	void *p = kmalloc_aligned(4096, 4096);
	if (p == NULL) {
		preboot_fail("paging: OOM allocating page-table page");
	}
	memset(p, 0, 4096);
	return (uint64_t)(uintptr_t)p;
}

[[maybe_unused]] static void pmm_free_page(uint64_t phys) {
	kfree(phys);
}

bool mmio_identity_map(uint64_t phys, uint64_t size) {
	return map_identity(phys, size, true, false, cache_uc, pmm_alloc_page);
}

bool ram_identity_map(uint64_t phys, uint64_t size, bool writable, bool executable) {
	return map_identity(phys, size, writable, executable, cache_wb, pmm_alloc_page);
}

bool identity_unmap(uint64_t phys, uint64_t size) {
	return unmap_range(phys, size);
}

void adopt_cloned_tables(void) {
	/* Refuse to run with interrupts enabled (IF bit set) */
	uint64_t rflags;
	__asm__ volatile ("pushfq; pop %0" : "=r"(rflags));
	if ((rflags & (1ull << 9)) != 0) {
		preboot_fail("adopt_cloned_tables: cannot clone with interrupts enabled");
	} else if (new_cr3) {
		preboot_fail("adopt_cloned_tables called on already-cloned cr3");
		return;
	}
	new_cr3 = pagetables_clone_deep(pmm_alloc_page);
	dprintf("Rug-pulling Limine's pagetables; new CR3=%p\n", (void*)new_cr3);
	pagetables_switch(new_cr3);
	dprintf("If you are reading this, the world didnt end!\n");
}

void adopt_cloned_tables_on_ap(void) {
	/* Refuse to run with interrupts enabled (IF=1) */
	uint64_t rflags;
	__asm__ volatile ("pushfq; pop %0" : "=r"(rflags));
	if ((rflags & (1ull << 9)) != 0) {
		preboot_fail("adopt_cloned_tables_on_ap: interrupts enabled");
	}

	if (new_cr3 == 0) {
		preboot_fail("adopt_cloned_tables_on_ap: new_cr3 not initialised");
	}

	/* Skip if this AP already uses the target CR3 */
	uint64_t cur_cr3;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cur_cr3));
	if (cur_cr3 == new_cr3) {
		dprintf("AP: CR3 already adopted (%p)\n", (void *)new_cr3);
		return;
	}

	/* Serialise then flip CR3 */
	__asm__ volatile ("" ::: "memory");
	__asm__ volatile ("mov %0, %%cr3" :: "r"(new_cr3) : "memory");

	dprintf("AP: adopted CR3=%p\n", (void *)new_cr3);
}


void walk_page_tables(uint64_t *table, uint64_t base_va, int level) {
	for (uint64_t i = 0; i < 512; i++) {
		uint64_t entry = table[i];
		if (!(entry & PTE_PRESENT)) continue;

		uint64_t addr = entry & 0x000ffffffffff000;
		uint64_t virt = base_va | (i << (39 - level * 9));

		// At PDP level (1), PS means 1GiB page
		// At PD  level (2), PS means 2MiB page
		// At PT  level (3), always a 4KiB mapping
		if ((level == 1 && (entry & PTE_PAGE_SIZE)) ||
		    (level == 2 && (entry & PTE_PAGE_SIZE)) ||
		    (level == 3)) {
			dump_mapping(virt, addr, entry, level);
		} else {
			uint64_t *next = (uint64_t *)(addr);
			walk_page_tables(next, virt, level + 1);
		}
	}
}

void validate_limine_page_tables_and_gdt(void) {
	__asm__ volatile("sgdt %0" : "=m"(gdtr));
	uint64_t *gdt = (uint64_t *)gdtr.base;
	int count = (gdtr.limit + 1) / 8;
	limine_gdtr = gdtr.base;

	dprintf("gdtr.base=%p gdtr.limit=%u count=%u\n", (void*)gdtr.base, gdtr.limit, count);
	for (int i = 0; i < count; i++) {
		dprintf("GDT[%d] = %016lx\n", i, gdt[i]);
	}

	uint64_t cr3;
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	dprintf("cr3=%p\n", (void*)cr3);
	limine_cr3 = cr3;
	walk_page_tables((uint64_t *)cr3, 0, 0);
}
