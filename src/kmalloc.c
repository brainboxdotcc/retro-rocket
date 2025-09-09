#include <kernel.h>
#include <kmalloc.h>
#include <limine.h>
#include <debugger.h>
#include <tinyalloc.h>

/*
 * 12mb kmalloc_low space for drivers that require space
 * guaranteed below 4gb
 */
#define LOW_HEAP_START 0x0800000 // Starts at 8mb
#define LOW_HEAP_MAX   0x1400000 // Ends at 20mb

static uint64_t heaplen   = 0;
static uint64_t allocated = 0;
static uint32_t low_mem_cur = LOW_HEAP_START;
static spinlock_t allocator_lock = 0;

volatile struct limine_memmap_request memory_map_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0,
};

void preboot_fail(const char* msg) {
	setforeground(COLOUR_LIGHTRED);
	kprintf("PANIC: %s\n", msg);
	backtrace();   // show stack trace
#ifdef PROFILE_KERNEL
	profile_dump();
#endif
	wait_forever();
}

static const char *limine_memmap_type_str(uint64_t type) {
	switch (type) {
		case LIMINE_MEMMAP_USABLE:                 return "Usable";
		case LIMINE_MEMMAP_RESERVED:               return "Reserved";
		case LIMINE_MEMMAP_ACPI_RECLAIMABLE:       return "ACPI Reclaimable";
		case LIMINE_MEMMAP_ACPI_NVS:               return "ACPI NVS";
		case LIMINE_MEMMAP_BAD_MEMORY:             return "Bad Memory";
		case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "Bootloader Reclaimable";
		case LIMINE_MEMMAP_KERNEL_AND_MODULES:     return "Kernel+Modules";
		case LIMINE_MEMMAP_FRAMEBUFFER:            return "Framebuffer";
		default:                                   return "Unknown";
	}
}

static void dump_limine_memmap(void) {
	uint64_t totals[8] = {0};
	uint64_t total_all = 0;
	dprintf("---- Limine memory map ----\n");
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; i++) {
		struct limine_memmap_entry *e = memory_map_request.response->entries[i];
		dprintf("base=%016lx, length=%016lx, type=%s\n", e->base, e->length, limine_memmap_type_str(e->type));
		if (e->type <= LIMINE_MEMMAP_FRAMEBUFFER) {
			totals[e->type] += e->length;
		}
		total_all += e->length;
	}
	dprintf("---------------------------\n");
	dprintf("Per-type totals:\n");
	dprintf("  Usable                 : %lu bytes\n", totals[LIMINE_MEMMAP_USABLE]);
	dprintf("  Reserved               : %lu bytes\n", totals[LIMINE_MEMMAP_RESERVED]);
	dprintf("  ACPI Reclaimable       : %lu bytes\n", totals[LIMINE_MEMMAP_ACPI_RECLAIMABLE]);
	dprintf("  ACPI NVS               : %lu bytes\n", totals[LIMINE_MEMMAP_ACPI_NVS]);
	dprintf("  Bad Memory             : %lu bytes\n", totals[LIMINE_MEMMAP_BAD_MEMORY]);
	dprintf("  Bootloader Reclaimable : %lu bytes\n", totals[LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE]);
	dprintf("  Kernel+Modules         : %lu bytes\n", totals[LIMINE_MEMMAP_KERNEL_AND_MODULES]);
	dprintf("  Framebuffer            : %lu bytes\n", totals[LIMINE_MEMMAP_FRAMEBUFFER]);
	dprintf("---------------------------\n");
	dprintf("  Grand total (all types): %lu bytes\n", total_all);
}


void init_heap(void) {
	uint64_t best_len  = 0;
	uint64_t best_addr = 0;
	int64_t  best_idx  = -1;
	bool lowheap_contained = false;

	init_spinlock(&allocator_lock);
	dump_limine_memmap();

	/* Find the largest usable entry, and validate LOW_HEAP_START/LOW_HEAP_MAX */
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		struct limine_memmap_entry *e = memory_map_request.response->entries[i];
		uint64_t base = e->base;
		uint64_t end  = e->base + e->length;
		uint64_t ovl_lo = (LOW_HEAP_START > base) ? LOW_HEAP_START : base;
		uint64_t ovl_hi = (LOW_HEAP_MAX   < end ) ? LOW_HEAP_MAX   : end;
		if (ovl_lo < ovl_hi) {
			if (e->type == LIMINE_MEMMAP_USABLE) {
				if (LOW_HEAP_START >= base && LOW_HEAP_MAX <= end) {
					lowheap_contained = true;
				}
			} else {
				dprintf("heap: ERROR low-heap overlaps non-usable [0x%016lx..0x%016lx] type=%s\n", base, end, limine_memmap_type_str(e->type));
				preboot_fail("LOW_HEAP overlaps a non-usable memmap entry");
			}
		}
		if (e->type == LIMINE_MEMMAP_USABLE && e->length > best_len) {
			best_len  = e->length;
			best_addr = e->base;
			best_idx  = (int64_t)i;
		}
	}

	if (!lowheap_contained) {
		dprintf("heap: ERROR low-heap [0x%016lx..0x%016lx] not fully inside any USABLE entry\n", (unsigned long)LOW_HEAP_START, (unsigned long)LOW_HEAP_MAX);
		preboot_fail("LOW_HEAP region outside usable RAM");
	}

	if (best_idx < 0 || !best_addr || best_len < 0x800000) {
		preboot_fail("No usable RAM found for heap");
	}

	/* Preserve existing behaviour for the primary heap:
	   carve the low heap bump area out with LOW_HEAP_MAX. */
	uint64_t heapstart = best_addr + LOW_HEAP_MAX;
	heaplen   = (best_len > LOW_HEAP_MAX) ? (best_len - LOW_HEAP_MAX) : 0;

	if (heaplen <= sizeof(ta_header)) {
		preboot_fail("Primary heap too small after LOW_HEAP_MAX carve-out");
	}

	ta_init((void *)heapstart, heaplen, 8);

	/* Log the primary region and start accumulating total usable bytes
	   (exclude allocator headers so this reflects payload capacity) */
	uint64_t total_usable = heaplen > sizeof(ta_header) ? (heaplen - sizeof(ta_header)) : 0;
	dprintf("heap: add primary region 0x%016lx..0x%016lx (%lu bytes)\n", heapstart, (heapstart + heaplen), heaplen);

	/* Pass 2: roll in every other usable region via ta_add_region(),
	   clipping out the low-heap window [LOW_HEAP_START, LOW_HEAP_END] */
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		if ((int64_t)i == best_idx) {
			continue; /* skip the one we already used for ta_init() */
		}

		struct limine_memmap_entry *entry = memory_map_request.response->entries[i];
		if (entry->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}

		uint64_t base = entry->base;
		uint64_t end  = entry->base + entry->length;

		/* Left segment: [base, min(end, LOW_HEAP_START)) */
		if (base < LOW_HEAP_START) {
			uint64_t seg_end = end < LOW_HEAP_START ? end : LOW_HEAP_START;
			if (seg_end > base) {
				uint64_t seg_len = seg_end - base;
				if (seg_len > sizeof(ta_header)) {
					if (ta_add_region((void *)base, (size_t)seg_len)) {
						dprintf("heap: add region 0x%016lx..0x%016lx (%lu bytes)\n", base, seg_end, seg_len);
						total_usable += (seg_len - sizeof(ta_header));
					}
				}
			}
		}

		/* Right segment: [max(base, LOW_HEAP_END), end) */
		if (end > LOW_HEAP_MAX) {
			uint64_t seg_base = base > LOW_HEAP_MAX ? base : LOW_HEAP_MAX;
			if (end > seg_base) {
				uint64_t seg_len = end - seg_base;
				/* If this happens to match the primary region exactly,
				   ta_add_region() will just link it; but we’ve already
				   skipped best_idx so overlap shouldn't occur here. */
				if (seg_len > sizeof(ta_header)) {
					if (ta_add_region((void *)seg_base, (size_t)seg_len)) {
						dprintf("heap: add region 0x%016lx..0x%016lx (%lu bytes)\n", seg_base, end, seg_len);
						total_usable += (seg_len - sizeof(ta_header));
					}
				}
			}
		}
	}

	heaplen = total_usable;
	dprintf("heap: total usable RAM for allocator: %lu bytes (%lu MB)\n", total_usable, total_usable / 1024 / 1024);

	dprintf_buffer_init(0);
}

void* kmalloc(uint64_t size) {
	uint64_t flags;
	lock_spinlock_irq(&allocator_lock, &flags);
	void* p = ta_alloc(size);
	allocated += ta_usable_size((void*)p);
	unlock_spinlock_irq(&allocator_lock, flags);
	return p;
}

void kfree(const void* ptr) {
	if (!ptr) {
		return;
	}
	uint64_t flags;
	lock_spinlock_irq(&allocator_lock, &flags);
	uintptr_t a = (uintptr_t)ptr;
	if (a >= LOW_HEAP_START && a < LOW_HEAP_MAX) {
		preboot_fail("kfree: tried to free low heap memory - use kfree_low instead!");
	}
	allocated -= ta_usable_size((void*)ptr);
	ta_free((void*)ptr);
	unlock_spinlock_irq(&allocator_lock, flags);
}

uint32_t kmalloc_low(uint32_t size) {
	uint64_t flags;
	lock_spinlock_irq(&allocator_lock, &flags);
	uint32_t ret = low_mem_cur;
	if (ret + size >= LOW_HEAP_MAX) {
		preboot_fail("kmalloc_low exhausted");
	}
	low_mem_cur += size;
	unlock_spinlock_irq(&allocator_lock, flags);
	return ret;
}


void kfree_low(uint32_t addr) {
	(void)addr; /* no-op */
}

void* kcalloc(size_t num, size_t size) {
	void* p = kmalloc(num * size);
	memset(p, 0, num * size);
	return p;
}

void* krealloc(void* ptr, size_t new_size) {
	if (!ptr) {
		return kmalloc(new_size);
	}
	void* new_ptr = kmalloc(new_size);
	memcpy(new_ptr, ptr, new_size);
	kfree(ptr);
	return new_ptr;
}

uint64_t get_free_memory() {
	return heaplen - allocated;
}

uint64_t get_used_memory() {
	return allocated;
}

uint64_t get_total_memory() {
	return heaplen;
}

void* kmalloc_aligned(uint64_t size, uint64_t align) {
	uintptr_t raw;
	uintptr_t aligned;
	void **save;

	if (align < 8 || (align & (align - 1)) != 0) {
		preboot_fail("kmalloc_aligned: alignment must be a power of two >= 8");
	}

	/* allocate enough for requested size + alignment slack + header */
	raw = (uintptr_t)kmalloc(size + align - 1 + sizeof(void*));
	if (!raw) {
		return NULL;
	}

	/* move past header first, then align */
	aligned = (raw + sizeof(void*) + (align - 1)) & ~(uintptr_t)(align - 1);

	/* stash raw just before the returned aligned pointer */
	save = (void**)aligned;
	save[-1] = (void*)raw;

	return (void*)aligned;
}

void kfree_aligned(const void* ptr) {
	if (!ptr) {
		return;
	}
	/* retrieve original raw pointer and free normally */
	void *raw = ((void**)ptr)[-1];
	kfree(raw);
}
