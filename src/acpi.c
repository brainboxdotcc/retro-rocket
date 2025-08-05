#include <kernel.h>
#include <uacpi/uacpi.h>
#include <uacpi/namespace.h>
#include <uacpi/resources.h>
#include <stdatomic.h>
#include "uacpi/context.h"

/**
 * @brief Yeah its memory hungry. Seems it prefers speed over ram use.
 * In testing on qemu 6.2.0, it took 15mb peak to parse and boot, so
 * this 64mb is a very generous amount to deal with real hardware's
 * beastly AML.
 */
#define UACPI_ARENA_SIZE (1024 * 1024 * 4)

volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0,
};

extern volatile struct limine_smp_request smp_request;

extern size_t aps_online;

buddy_allocator_t acpi_pool = { 0 };
void* uacpi_region = NULL;

static uint8_t lapic_ids[256] = {0}; // CPU core Local APIC IDs
static uint8_t ioapic_ids[256] = {0}; // CPU core Local APIC IDs
static uint16_t numcore = 0;         // number of cores detected
static uint16_t numioapic = 0;
static uint64_t lapic_ptr = 0;       // pointer to the Local APIC MMIO registers
static uint64_t ioapic_ptr[256] = {0};      // pointer to the IO APIC MMIO registers
static uint32_t ioapic_gsi_base[256] = {0};
static uint8_t ioapic_gsi_count[256] = {0};

pci_irq_route_t pci_irq_routes[MAX_PCI_ROUTES] = {};

uint32_t cpu_id_mapping[MAX_CPUS] = { 0 };

uint64_t mhz = 0, tsc_per_sec = 1;

void enumerate_all_gsis(void);

uint32_t get_lapic_id_from_cpu_id(uint8_t cpu_id) {
	return cpu_id_mapping[cpu_id];
}

uint8_t get_cpu_id_from_lapic_id(uint32_t lapic_id) {
	for (uint8_t x = 0; x < MAX_CPUS - 1; ++x) {
		if (cpu_id_mapping[x] == lapic_id) {
			return x;
		}
	}
	return INVALID_CPU_ID;
}

void set_lapic_id_for_cpu_id(uint8_t cpu_id, uint32_t lapic_id) {
	cpu_id_mapping[cpu_id] = lapic_id;
}

void init_uacpi(void) {
	uacpi_status st;

	time_t start_time = time(NULL);
	while (time(NULL) == start_time); // settle
	uint64_t start_tsc = rdtsc();
	start_time = time(NULL);
	while (time(NULL) == start_time); // spin
	uint64_t end_tsc = rdtsc();
	tsc_per_sec = end_tsc - start_tsc;
	mhz = tsc_per_sec / 1000000;
	dprintf("mhz = %lu, tsc_per_sec = %lu\n", mhz, tsc_per_sec);

	uacpi_region = kmalloc(UACPI_ARENA_SIZE);
	if (!uacpi_region) {
		preboot_fail("Cannot claim 64mb uACPI arena");
	}
	buddy_init(&acpi_pool, uacpi_region, 6, 22);

	uacpi_context_set_log_level(UACPI_LOG_INFO);
	st = uacpi_initialize(0);
	if (uacpi_unlikely_error(st)) {
		preboot_fail("uACPI init failed");
	}

	st = uacpi_namespace_load();
	if (uacpi_unlikely_error(st)) {
		preboot_fail("uACPI namespace load failed");
	}

	st = uacpi_namespace_initialize();
	if (uacpi_unlikely_error(st)) {
		preboot_fail("uACPI namespace init failed");
	}
	dprintf("init_uacpi done. Peak allocation: %lu Current allocation: %lu\n", acpi_pool.peak_bytes, acpi_pool.current_bytes);
}

void delay_ns(uint64_t ns) {
	uint64_t ticks = (tsc_per_sec * ns) / 1e9;
	uint64_t start = rdtsc();
	while (rdtsc() - start < ticks);
}

uint64_t uacpi_kernel_get_nanoseconds_since_boot(void) {
	return (rdtsc() * 1000000000ULL) / tsc_per_sec;
}

rsdp_t *get_rsdp() {
	return (rsdp_t *) rsdp_request.response->address;
}

sdt_header_t *get_sdt_header() {
	rsdp_t *rsdp = get_rsdp();
	return (sdt_header_t *) (uint64_t) rsdp->rsdt_address;
}

uint8_t *get_lapic_ids() {
	return lapic_ids;
}

uint16_t get_cpu_count() {
	return numcore;
}

uint64_t get_local_apic() {
	return (uint64_t) lapic_ptr;
}

uint16_t get_ioapic_count() {
	return numioapic;
}

ioapic_t get_ioapic(uint16_t index) {
	ioapic_t ioapic = {0};
	if (index >= numioapic) {
		return ioapic;
	}
	ioapic.gsi_base = ioapic_gsi_base[index];
	ioapic.gsi_count = ioapic_gsi_count[index];
	ioapic.id = ioapic_ids[index];
	ioapic.paddr = ioapic_ptr[index];
	return ioapic;
}

void init_acpi() {
	uint8_t *ptr, *ptr2;
	uint32_t len;
	uint8_t *rsdt = (uint8_t *) get_sdt_header();
	numcore = 0;
	numioapic = 0;
	rr_flip();

	init_uacpi();
	rr_flip();

	// Iterate on ACPI table pointers
	for (len = *((uint32_t *) (rsdt + 4)), ptr2 = rsdt + 36; ptr2 < rsdt + len; ptr2 += rsdt[0] == 'X' ? 8 : 4) {
		ptr = (uint8_t *) (uint64_t) (rsdt[0] == 'X' ? *((uint64_t *) ptr2) : *((uint32_t *) ptr2));
		if (*ptr == 'A' && *(ptr + 1) == 'P' && *(ptr + 2) == 'I' && *(ptr + 3) == 'C') {
			// found MADT
			lapic_ptr = (uint64_t) (*((uint32_t *) (ptr + 0x24)));
			kprintf("Detected: 32-Bit Local APIC [base: %lx]\n", lapic_ptr);
			ptr2 = ptr + *((uint32_t *) (ptr + 4));
			// iterate on variable length records
			for (ptr += 44; ptr < ptr2; ptr += ptr[1]) {
				switch (ptr[0]) {
					case 0:
						if (ptr[4] & 1) {
							lapic_ids[numcore++] = ptr[3];
						}
						break; // found Processor Local APIC
					case 1:
						ioapic_ptr[numioapic] = (uint64_t) *((uint32_t *) (ptr + 4));
						ioapic_ids[numioapic] = ptr[2];
						ioapic_gsi_base[numioapic] = (uint32_t) *((uint32_t *) (ptr + 8));
						uint32_t *mmio = (uint32_t *) ioapic_ptr[numioapic];
						mmio[0] = 0x01;
						uint32_t count = ((mmio[0x10 / 4]) & 0xFF) + 1;
						ioapic_gsi_count[numioapic] = count;
						kprintf("Detected: IOAPIC [base: %lx; id: %d gsi base: %d gsi count: %d]\n",
							ioapic_ptr[numioapic], ioapic_ids[numioapic],
							ioapic_gsi_base[numioapic], count);
						numioapic++;
						break;  // found IOAPIC
					case 2: {
						madt_override_t *ovr = (madt_override_t *) ptr;
						kprintf("Interrupt override: IRQ %d -> GSI %d (flags: 0x%x)\n",
							ovr->irq_source, ovr->gsi, ovr->flags);
						pci_irq_route_t * r = &pci_irq_routes[ovr->irq_source];
						r->exists = true;
						r->int_pin = 0;
						r->gsi = ovr->gsi;
						r->polarity = (ovr->flags & 0x2) ? 1 : 0;
						r->trigger = (ovr->flags & 0x8) ? 1 : 0;
						r->detected_from = FROM_MADT;
						break;
					}
					case 5:
						lapic_ptr = *((uint64_t *) (ptr + 4));
						kprintf("Detected: 64-Bit Local APIC [base: %lx]\n", lapic_ptr);
						break;             // found 64 bit LAPIC
				}
			}
			break;
		}
	}
	enumerate_all_gsis();
	rr_flip();
	/* Fallback to just irq == gsi for anything not detected in MADT or _PRT */
	dprintf("Full IRQ->GSI MAP\n");
	for (int i = 0; i < 256; ++i) {
		pci_irq_route_t * r = &pci_irq_routes[i];
		if (r->exists == false) {
			r->exists = true;
			r->int_pin = 0;
			r->gsi = i;
			r->polarity = 0;
			r->trigger = 0;
			r->detected_from = FROM_FALLBACK;
		}
		if (r->exists && i > 0 && r->gsi == 0) {
			r->gsi = i;
		}
		if (i < 24) {
			dprintf("IRQ %d GSI %d PIN %d POLARITY %d TRIGGER %d FROM: %s\n", i, r->gsi, r->int_pin, r->polarity, r->trigger,
				(r->detected_from == FROM_MADT ? "_MADT" : (r->detected_from == FROM_PRT ? "_PRT" : "FALLBACK")));
		}
	}
	for (int i = 0; i < 16; ++i) {
		dprintf("IRQ %d maps to GSI %d\n", i, irq_to_gsi(i));
	}
	rr_flip();
}

void boot_aps() {
	if (numcore > 0) {
		kprintf("SMP: %d cores, %d IOAPICs\n", numcore, numioapic);
		if (!smp_request.response) {
			kprintf("No SMP response, running uniprocessor.\n");
			return;
		}

		uint64_t limit = smp_request.response->cpu_count;
		if (limit > MAX_CPUS - 1) {
			kprintf("WARNING: Your system has more than 254 CPUs; only 254 will be enabled\n");
			limit = MAX_CPUS - 1;
		}
		for (uint64_t i = 0; i < limit; i++) {
			struct limine_smp_info *cpu = smp_request.response->cpus[i];
			if (cpu->processor_id < 255) {
				set_lapic_id_for_cpu_id(cpu->processor_id, cpu->lapic_id);
			}
			if (cpu->lapic_id == smp_request.response->bsp_lapic_id || cpu->processor_id > 254) {
				if (cpu->lapic_id == smp_request.response->bsp_lapic_id) {
					kprintf("CPU: %d online; ID: %d\n", cpu->processor_id, cpu->lapic_id);
				}
				// Skip BSP and IDs over 254 (255 is broadcast, 256+ are too big for our array)
				continue;
			}
			cpu->goto_address = kmain_ap;
		}
		while (atomic_load(&aps_online) < limit - 1) {
			_mm_pause();
		}
	}
	kfree_null(&uacpi_region);
}

uint32_t irq_to_gsi(uint8_t irq) {
	pci_irq_route_t * r = &pci_irq_routes[irq];
	return r->gsi;
}

uint8_t get_irq_polarity(uint8_t irq) {
	pci_irq_route_t * r = &pci_irq_routes[irq];
	return r->polarity;
}

uint8_t get_irq_trigger_mode(uint8_t irq) {
	pci_irq_route_t * r = &pci_irq_routes[irq];
	return r->trigger;
}

typedef struct {
	pci_dev_t dev;
} uacpi_pci_dev_wrapper_t;

// Logging
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {
	dprintf("[uACPI] %s", msg);
}

// Memory allocation
void *uacpi_kernel_alloc(uacpi_size size) {
	return buddy_malloc(&acpi_pool, size);
}

void uacpi_kernel_free(void *ptr) {
	buddy_free(&acpi_pool, ptr);
}

// Physical memory mapping (flat model)
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
	return (void *) (uintptr_t) addr; // identity-mapped
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
	// no-op in flat memory
}

void uacpi_kernel_stall(uacpi_u8 usec) {
	delay_ns(usec * 1000);
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
	delay_ns(msec * 1000000);
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) {
	uacpi_pci_dev_wrapper_t *wrapper = uacpi_kernel_alloc(sizeof(uacpi_pci_dev_wrapper_t));
	if (!wrapper) return UACPI_STATUS_OUT_OF_MEMORY;

	wrapper->dev.bits = 0;
	wrapper->dev.bus_num = address.bus;
	wrapper->dev.device_num = address.device;
	wrapper->dev.function_num = address.function;

	*out_handle = wrapper;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
	// For I/O ports, mapping isn't needed — just pass the base through.
	*out_handle = (uacpi_handle) base;
	return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
	// No-op for I/O ports
	(void) handle;
}

uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value) {
	*out_value = inb((uintptr_t) handle + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value) {
	*out_value = inw((uintptr_t) handle + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value) {
	*out_value = inl((uintptr_t) handle + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 value) {
	outb((uintptr_t) handle + offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 value) {
	outw((uintptr_t) handle + offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 value) {
	outl((uintptr_t) handle + offset, value);
	return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
	uacpi_kernel_free(handle);
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	*value = (uacpi_u8) (pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	*value = (uacpi_u16) (pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	*value = (uacpi_u32) (pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

static uacpi_handle last_handle = 1;

// Stub mutex/event/threading
uacpi_handle uacpi_kernel_create_mutex(void) { return (uacpi_handle) last_handle++; }

void uacpi_kernel_free_mutex(uacpi_handle h) {}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16) { return UACPI_STATUS_OK; }

void uacpi_kernel_release_mutex(uacpi_handle) {}

uacpi_handle uacpi_kernel_create_event(void) { return (uacpi_handle) last_handle++; }

void uacpi_kernel_free_event(uacpi_handle h) {}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) { return true; }

void uacpi_kernel_signal_event(uacpi_handle) {}

void uacpi_kernel_reset_event(uacpi_handle) {}

uacpi_thread_id uacpi_kernel_get_thread_id(void) { return 1; }

// Optional no-op spinlock support
uacpi_handle uacpi_kernel_create_spinlock(void) { return (uacpi_handle) last_handle++; }

void uacpi_kernel_free_spinlock(uacpi_handle) {}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle) { return 0; }

void uacpi_kernel_unlock_spinlock(uacpi_handle, uacpi_cpu_flags) {}

// RSDP (you already have this)
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
	*out_rsdp_address = rsdp_request.response->address;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_install_interrupt_handler(
	uacpi_u32 irq,
	uacpi_interrupt_handler handler,
	uacpi_handle ctx,
	uacpi_handle *out_irq_handle
) {
	(void) irq;
	(void) handler;
	(void) ctx;
	*out_irq_handle = last_handle++;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
	uacpi_interrupt_handler handler,
	uacpi_handle irq_handle
) {
	(void) handler;
	(void) irq_handle;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
	switch (req->type) {
		case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
			dprintf("uACPI breakpoint: AML requested a breakpoint\n");
			return UACPI_STATUS_OK;

		case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
			preboot_fail("uACPI fatal AML error");
			break;

		default:
			return UACPI_STATUS_INTERNAL_ERROR;
	}
	return UACPI_STATUS_OK;
}


void async_run_gpe_handler(uacpi_handle gpe) {
	(void) gpe;
	// No GPE support yet
}

uacpi_status uacpi_kernel_schedule_work(
	uacpi_work_type type,
	uacpi_work_handler handler,
	uacpi_handle ctx
) {
	(void) type;
	(void) ctx;

	// Synchronously call it immediately (not technically correct, but fine for now)
	handler(ctx);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
	// No deferred work in Retro Rocket yet
	return UACPI_STATUS_OK;
}

const char *triggering_str(uint8_t trig) {
	return trig == 0 ? "Edge" : "Level";
}

const char *polarity_str(uint8_t pol) {
	return pol == 0 ? "High" : "Low";
}

const char *sharing_str(uint8_t share) {
	return share == 0 ? "Exclusive" : "Shared";
}

static uacpi_iteration_decision resource_callback(void *user, uacpi_resource *res) {
	if (res->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		uacpi_resource_extended_irq *irq = &res->extended_irq;
		for (uacpi_u32 i = 0; i < irq->num_irqs; ++i) {
			dprintf("GSI (EXT_IRQ): %u | Trigger: %s | Polarity: %s | Sharing: %s | Wake: %u | Source: %s\n",
				irq->irqs[i], triggering_str(irq->triggering), polarity_str(irq->polarity), sharing_str(irq->sharing),
				irq->wake_capability, irq->source.length ? irq->source.string : "<none>"
			);
			pci_irq_route_t * r = &pci_irq_routes[irq->irqs[i]];
			r->exists = true;
			r->polarity = irq->polarity;
			r->trigger = irq->triggering;
			r->detected_from = FROM_PRT;
		}
	} else if (res->type == UACPI_RESOURCE_TYPE_IRQ) {
		uacpi_resource_irq *irq = &res->irq;
		for (uacpi_u32 i = 0; i < irq->num_irqs; ++i) {
			dprintf("IRQ (Legacy): %u | Trigger: %s | Polarity: %s | Sharing: %s | Wake: %u\n",
				irq->irqs[i], triggering_str(irq->triggering), polarity_str(irq->polarity), sharing_str(irq->sharing), irq->wake_capability
			);
			pci_irq_route_t * r = &pci_irq_routes[irq->irqs[i]];
			r->exists = true;
			r->gsi = irq->irqs[i];
			r->polarity = irq->polarity;
			r->trigger = irq->triggering;
			r->detected_from = FROM_PRT;
		}
	}

	return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_iteration_decision device_callback(void *user, uacpi_namespace_node *node, uacpi_u32 depth) {
	(void)user;
	(void)depth;

	uacpi_resources *resources = NULL;
	if (uacpi_get_current_resources(node, &resources) != UACPI_STATUS_OK || !resources)
		return UACPI_ITERATION_DECISION_CONTINUE;

	uacpi_for_each_resource(resources, resource_callback, NULL);

	uacpi_free_resources(resources);
	return UACPI_ITERATION_DECISION_CONTINUE;
}

void enumerate_all_gsis(void) {
	uacpi_namespace_for_each_child(
		uacpi_namespace_root(), device_callback,
		NULL, UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY, NULL
	);
}
