#include <kernel.h>

#define PRESENT   (1ULL << 0)
#define WRITE     (1ULL << 1)
#define NX        (1ULL << 63)
#define PS        (1ULL << 7)

static uint64_t limine_gdtr;
static uint64_t limine_cr3;

struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdtr;

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
