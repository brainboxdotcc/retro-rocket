#include <kernel.h>
#include <kmalloc.h>
#include <limine.h>
#include <debugger.h>
#include <allocator.h>

/*
 * 12mb kmalloc_low space for drivers that require space
 * guaranteed below 4gb
 */
static uint32_t LOW_HEAP_START;
static uint32_t LOW_HEAP_MAX;

static uint64_t heaplen   = 0;
static uint64_t allocated = 0;
static uint32_t low_mem_cur = 0;
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

	/* Find a 12 MB USABLE window below 4 GB for LOW_HEAP */
	const uint64_t LOW_HEAP_SIZE    = 1024 * 1024 * 12;
	const uint64_t FOUR_GB_BOUNDARY = (1ull << 32);
	bool lowheap_found = false;

	init_spinlock(&allocator_lock);
	dump_limine_memmap();

	/* Pick LOW_HEAP window (any LOW_HEAP_SIZE USABLE below 4 GB) */
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		struct limine_memmap_entry *e = memory_map_request.response->entries[i];
		if (e->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}
		uint64_t base = e->base;
		uint64_t end  = e->base + e->length;
		if (base >= FOUR_GB_BOUNDARY) {
			continue;
		}
		if (end > FOUR_GB_BOUNDARY) {
			end = FOUR_GB_BOUNDARY;
		}
		if (end > base && (end - base) >= LOW_HEAP_SIZE) {
			uint64_t start = (base + 0xFFF) & ~0xFFFull; /* 4 KB align */
			if ((end - start) >= LOW_HEAP_SIZE) {
				LOW_HEAP_START = start;
				LOW_HEAP_MAX   = start + LOW_HEAP_SIZE;
				lowheap_found  = true;
				dprintf("heap: LOW_HEAP selected at [0x%08x..0x%08x] (%lu MB)\n", LOW_HEAP_START, LOW_HEAP_MAX, LOW_HEAP_SIZE / 1024 / 1024);
				break;
			}
		}
	}

	if (!lowheap_found) {
		char error[256];
		snprintf(error, 255, "Unable to find %lu MB USABLE below 4 GB for LOW_HEAP", LOW_HEAP_SIZE / 1024 / 1024);
		preboot_fail(error);
	}

	/* Find the largest USABLE entry for primary ta_init() */
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		struct limine_memmap_entry *e = memory_map_request.response->entries[i];
		if (e->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}
		if (e->length > best_len) {
			best_len  = e->length;
			best_addr = e->base;
			best_idx  = (int64_t)i;
		}
	}

	if (best_idx < 0 || !best_addr || best_len < 0x800000) {
		preboot_fail("No usable RAM found for heap");
	}

	low_mem_cur = LOW_HEAP_START;

	/* Build the primary heap region, excluding LOW_HEAP if it overlaps. */
	uint64_t bbase = best_addr;
	uint64_t bend  = best_addr + best_len;

	uint64_t left_lo  = bbase;
	uint64_t left_hi  = (LOW_HEAP_START > bbase) ? (LOW_HEAP_START < bend ? LOW_HEAP_START : bend) : bbase;
	uint64_t right_lo = (LOW_HEAP_MAX > bbase) ? (LOW_HEAP_MAX < bend ? LOW_HEAP_MAX : bend) : bbase;
	uint64_t right_hi = bend;

	uint64_t left_len  = (left_hi  > left_lo)  ? (left_hi  - left_lo)  : 0;
	uint64_t right_len = (right_hi > right_lo) ? (right_hi - right_lo) : 0;

	uint64_t heapstart, heaplen_local;
	if (left_len >= right_len) {
		heapstart = left_lo;
		heaplen_local = left_len;
	} else {
		heapstart = right_lo;
		heaplen_local = right_len;
	}

	if (heaplen_local <= sizeof(allocator_header)) {
		preboot_fail("Primary heap too small after LOW_HEAP exclusion");
	}

	allocator_init((void *) heapstart, heaplen_local, 8);

	uint64_t total_usable = heaplen_local > sizeof(allocator_header) ? (heaplen_local - sizeof(allocator_header)) : 0;
	dprintf("heap: add primary region 0x%016lx..0x%016lx (%lu bytes)\n", heapstart, (heapstart + heaplen_local), heaplen_local);

	/* Add other USABLE regions, clipping out LOW_HEAP */
	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		if ((int64_t)i == best_idx) {
			continue;
		}

		struct limine_memmap_entry *entry = memory_map_request.response->entries[i];
		if (entry->type != LIMINE_MEMMAP_USABLE) {
			continue;
		}

		uint64_t base = entry->base;
		uint64_t end  = entry->base + entry->length;

		if (base < LOW_HEAP_START) {
			uint64_t seg_end = end < LOW_HEAP_START ? end : LOW_HEAP_START;
			if (seg_end > base) {
				uint64_t seg_len = seg_end - base;
				if (seg_len > sizeof(allocator_header)) {
					if (allocator_add_region((void *) base, (size_t) seg_len)) {
						dprintf("heap: add region 0x%016lx..0x%016lx (%lu bytes)\n", base, seg_end, seg_len);
						total_usable += (seg_len - sizeof(allocator_header));
					}
				}
			}
		}

		if (end > LOW_HEAP_MAX) {
			uint64_t seg_base = base > LOW_HEAP_MAX ? base : LOW_HEAP_MAX;
			if (end > seg_base) {
				uint64_t seg_len = end - seg_base;
				if (seg_len > sizeof(allocator_header)) {
					if (allocator_add_region((void *) seg_base, (size_t) seg_len)) {
						dprintf("heap: add region 0x%016lx..0x%016lx (%lu bytes)\n", seg_base, end, seg_len);
						total_usable += (seg_len - sizeof(allocator_header));
					}
				}
			}
		}
	}

	/* Selectively reclaim Bootloader Reclaimable */
	struct reqset rs = request_addresses();
	const size_t BLR_MIN_RECLAIM = (1u << 20); /* 1 MB */
	size_t remaining_blr = 0;

	for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
		struct limine_memmap_entry *e = memory_map_request.response->entries[i];
		if (e->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			continue;
		}

		uint64_t rbase = e->base;
		uint64_t rend  = e->base + e->length;

		if (e->length < BLR_MIN_RECLAIM) {
			dprintf("heap: keep BLR 0x%016lx..0x%016lx (small: %lu bytes)\n", rbase, rend, e->length);
			remaining_blr += e->length;
			continue;
		}

		uintptr_t hit = 0;
		for (size_t j = 0; j < rs.count; ++j) {
			uintptr_t p = rs.ptrs[j];
			if (p && p >= rbase && p < rend) {
				hit = p;
				break;
			}
		}

		if (hit) {
			dprintf("heap: keep BLR 0x%016lx..0x%016lx (contains 0x%016lx)\n", rbase, rend, hit);
			remaining_blr += e->length;
			continue;
		}

		if (e->length > sizeof(allocator_header)) {
			if (allocator_add_region((void *) rbase, (size_t) e->length)) {
				dprintf("heap: reclaim BLR 0x%016lx..0x%016lx (%lu bytes)\n", rbase, rend, e->length);
				total_usable += (e->length - sizeof(allocator_header));
			} else {
				dprintf("heap: WARNING failed to add BLR 0x%016lx..0x%016lx\n", rbase, rend);
				remaining_blr += e->length;
			}
		} else {
			dprintf("heap: skip tiny BLR 0x%016lx..0x%016lx (%lu bytes)\n", rbase, rend, e->length);
			remaining_blr += e->length;
		}
	}

	/* Now we're done. We've clawed back every last byte we can */
	heaplen = total_usable;
	dprintf("heap: total usable RAM for allocator: %lu bytes (%lu MB); remaining bootloader reserved: %lu MB\n", total_usable, total_usable / 1024 / 1024, remaining_blr / 1024 / 1024);

	dprintf_buffer_init(0);
}


void* kmalloc(uint64_t size) {
	uint64_t flags;
	lock_spinlock_irq(&allocator_lock, &flags);
	void* p = allocator_alloc(size);
	allocated += allocator_usable_size((void *) p);
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
	allocated -= allocator_usable_size((void *) ptr);
	allocator_free((void *) ptr);
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
		dprintf("kmalloc_aligned: alignment must be a power of two >= 8. Requested alignment: %lu\n", align);
		align = 8;
	}

	/* allocate enough for requested size + alignment slack + header */
	raw = kmalloc(size + align - 1 + sizeof(void*));
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
