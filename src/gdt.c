#include <kernel.h>

#define PRESENT   (1ULL << 0)
#define WRITE     (1ULL << 1)
#define NX        (1ULL << 63)
#define PS        (1ULL << 7)

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

struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdtr;

static void dump_mapping(uint64_t virt, uint64_t phys, uint64_t flags, int level) {
	const char *sizes[] = { "4K", "2M", "1G" };
	if (3 - level == 2) {
		dprintf("VA 0x%lx -> PA 0x%lx [%s%s%s%s] (%s)\n",
			virt, phys,
			flags & PRESENT ? "P" : "-",
			flags & WRITE ? "W" : "-",
			flags & NX ? "NX" : "-",
			flags & PS ? "PS" : "--",
			sizes[3 - level]);
	}
}

void walk_page_tables(uint64_t *table, uint64_t base_va, int level) {
	for (uint64_t i = 0; i < 512; i++) {
		uint64_t entry = table[i];
		if (!(entry & PRESENT)) continue;

		uint64_t addr = entry & 0x000ffffffffff000;
		uint64_t virt = base_va | (i << (39 - level * 9));

		// At PDP level (1), PS means 1GiB page
		// At PD  level (2), PS means 2MiB page
		// At PT  level (3), always a 4KiB mapping
		if ((level == 1 && (entry & PS)) ||
		    (level == 2 && (entry & PS)) ||
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

	for (int i = 0; i < count; i++) {
		dprintf("GDT[%d] = %016lx\n", i, gdt[i]);
	}

	uint64_t cr3;
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	walk_page_tables((uint64_t *)cr3, 0, 0);
}
