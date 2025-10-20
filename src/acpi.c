#include <kernel.h>
#include <uacpi/uacpi.h>
#include <uacpi/acpi.h>
#include <uacpi/helpers.h>
#include <uacpi/namespace.h>
#include <uacpi/resources.h>
#include <stdatomic.h>
#include "uacpi/context.h"
#include "uacpi/tables.h"
#include "uacpi/sleep.h"
#include "uacpi/event.h"

volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0,
};

extern volatile struct limine_smp_request smp_request;

extern size_t aps_online;

buddy_allocator_t acpi_pool = { 0 };

static bool uacpi_claim_phase = false;

static uint64_t pm_timer_port = 0;
static bool pm_timer_32bit = 0;
static bool pm_timer_is_io = 0;
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

typedef struct uacpi_irq_req {
	uint32_t gsi;				/* GSI from uACPI (not a vector). */
	uacpi_interrupt_handler handler;	/* uACPI handler: handler(ctx). */
	uacpi_handle ctx;
	uint8_t vector;				/* Bound vector once claimed. */
	bool claimed;
} uacpi_irq_req;

#define UACPI_DEFER_MAX 16
static uacpi_irq_req *uacpi_deferred[UACPI_DEFER_MAX];
static size_t uacpi_deferred_count = 0;

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

	buddy_init(&acpi_pool, 6, 22, 22);

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

	struct acpi_fadt *fadt = NULL;
	st = uacpi_table_fadt(&fadt);
	if (!uacpi_unlikely_error(st) && fadt) {
		struct acpi_gas *gas = &fadt->x_pm_tmr_blk;

		if (gas->address_space_id == 1) {
			// System I/O
			pm_timer_port = (uint16_t)gas->address;
			pm_timer_is_io = true;
			dprintf("Using PM timer IO port 0x%04lx\n", pm_timer_port);
		} else if (gas->address_space_id == 0) {
			// System memory (MMIO)
			pm_timer_port = (uintptr_t)gas->address;
			pm_timer_is_io = false;
			dprintf("Using PM timer MMIO 0x%lx\n", pm_timer_port);
		} else {
			dprintf("Unsupported PM timer space_id %u\n", gas->address_space_id);
			pm_timer_port = 0;
		}

		pm_timer_32bit = (gas->register_bit_width == 32);
	} else {
		dprintf("FADT not found, no PM timer available\n");
		pm_timer_port = 0;
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
	uint8_t *rsdt = (uint8_t *) get_sdt_header();
	numcore = 0;
	numioapic = 0;
	rr_flip();

	init_uacpi();
	rr_flip();

	// Iterate on ACPI table pointers
	// First 4 bytes after the table header are the length
	uint32_t table_len = *((uint32_t *)(rsdt + 4));

	// Work out entry size (RSDT = 32‑bit, XSDT = 64‑bit)
	bool is_xsdt = (rsdt[0] == 'X');
	size_t entry_size = is_xsdt ? 8 : 4;

	// Entry count = (total length - header size) / entry size
	// ACPI header is always 36 bytes
	size_t entry_count = (table_len - 36) / entry_size;

	// Start of entry array
	uint8_t* entry_ptr = rsdt + 36;

	for (size_t i = 0; i < entry_count; i++) {
		uint64_t addr;
		if (is_xsdt) {
			addr = *((uint64_t *)(entry_ptr + i * entry_size));
		} else {
			addr = *((uint32_t *)(entry_ptr + i * entry_size));
		}

		uint8_t *acpi_table = (uint8_t *)(uintptr_t)addr;

		if (memcmp(acpi_table, "APIC", 4) == 0) {
			// found MADT
			lapic_ptr = (uint64_t) (*((uint32_t *) (acpi_table + 0x24)));
			dprintf("Detected: 32-Bit Local APIC [base: %lx]\n", lapic_ptr);
			uint8_t* madt_end = acpi_table + *((uint32_t *) (acpi_table + 4));
			// iterate on variable length records
			for (acpi_table += 44; acpi_table < madt_end; acpi_table += acpi_table[1]) {
				switch (acpi_table[0]) {
					case 0:
						if (acpi_table[4] & 1) {
							lapic_ids[numcore++] = acpi_table[3];
						}
						break; // found Processor Local APIC
					case 1:
						ioapic_ptr[numioapic] = (uint64_t) *((uint32_t *) (acpi_table + 4));
						ioapic_ids[numioapic] = acpi_table[2];
						ioapic_gsi_base[numioapic] = (uint32_t) *((uint32_t *) (acpi_table + 8));
						uint32_t *mmio = (uint32_t *) ioapic_ptr[numioapic];
						mmio[0] = 0x01;
						uint32_t count = ((mmio[0x10 / 4]) & 0xFF) + 1;
						ioapic_gsi_count[numioapic] = count;
						dprintf("Detected: IOAPIC [base: %lx; id: %d gsi base: %d gsi count: %d]\n",
							ioapic_ptr[numioapic], ioapic_ids[numioapic],
							ioapic_gsi_base[numioapic], count);
						numioapic++;
						break;  // found IOAPIC
					case 2: {
						madt_override_t *ovr = (madt_override_t *) acpi_table;
						dprintf("Interrupt override: IRQ %d -> GSI %d (flags: 0x%x)\n",
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
						lapic_ptr = *((uint64_t *) (acpi_table + 4));
						dprintf("Detected: 64-Bit Local APIC [base: %lx]\n", lapic_ptr);
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

uint32_t pm_timer_read(void) {
	if (pm_timer_port == 0) {
		return 0; // not available
	}

	uint32_t mask = pm_timer_32bit ? 0xFFFFFFFF : 0xFFFFFF;

	if (pm_timer_is_io) {
		// Always use a 32-bit port read
		return inl((uint16_t)pm_timer_port) & mask;
	} else {
		// MMIO read
		volatile uint32_t *reg = (volatile uint32_t *)(uintptr_t)pm_timer_port;
		return *reg & mask;
	}
}


bool pm_timer_available(void) {
	return pm_timer_port != 0;
}

bool pm_timer_is_32_bit(void) {
	return pm_timer_32bit;
}

void boot_aps() {
	if (numcore > 0) {
		dprintf("SMP: %d cores, %d IOAPICs\n", numcore, numioapic);
		if (!smp_request.response) {
			dprintf("No SMP response, running uniprocessor.\n");
			return;
		}

		uint64_t limit = smp_request.response->cpu_count;
		if (limit > MAX_CPUS - 1) {
			kprintf("WARNING: Your system has more than 254 CPUs; only 254 will be enabled\n");
			limit = MAX_CPUS - 1;
		}
		kprintf("CPUs startup: ");
		for (uint64_t i = 0; i < limit; i++) {
			struct limine_smp_info *cpu = smp_request.response->cpus[i];
			if (cpu->processor_id < 255) {
				set_lapic_id_for_cpu_id(cpu->processor_id, cpu->lapic_id);
			}
			if (cpu->lapic_id == smp_request.response->bsp_lapic_id || cpu->processor_id > 254) {
				if (cpu->lapic_id == smp_request.response->bsp_lapic_id) {
					kprintf("0 ");
				}
				// Skip BSP and IDs over 254 (255 is broadcast, 256+ are too big for our array)
				continue;
			}
			cpu->goto_address = kmain_ap;
		}
		while (atomic_load(&aps_online) < limit - 1) {
			__builtin_ia32_pause();
		}
		kprintf("\n");
	}
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
	mmio_identity_map(addr, len);
	return (void *) (uintptr_t) addr;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
	identity_unmap(addr, len);
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
	// For I/O ports, mapping isn't needed - just pass the base through.
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
	*value = pci_read8(wrapper->dev, offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	*value = pci_read16(wrapper->dev, offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	*value = pci_read32(wrapper->dev, offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write8(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write16(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *) handle;
	pci_write32(wrapper->dev, offset, value);
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

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
	return 1;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
	spinlock_t *lock = (spinlock_t *)buddy_malloc(&acpi_pool, sizeof(spinlock_t));
	if (lock == NULL) {
		return (uacpi_handle)0;
	}
	init_spinlock(lock);
	return (uacpi_handle)(uintptr_t)lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
	spinlock_t *lock = (spinlock_t *)(uintptr_t)handle;
	if (lock == NULL) {
		return;
	}
	__atomic_store_n(lock, 0, __ATOMIC_RELEASE);
	buddy_free(&acpi_pool, lock);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
	spinlock_t *lock = (spinlock_t *)(uintptr_t)handle;
	uint64_t saved_flags = 0;
	if (lock == NULL) {
		return 0;
	}
	lock_spinlock_irq(lock, &saved_flags);
	return saved_flags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
	spinlock_t *lock = (spinlock_t *)(uintptr_t)handle;
	if (lock == NULL) {
		return;
	}
	unlock_spinlock_irq(lock, flags);
}

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
	*out_rsdp_address = rsdp_request.response->address;
	return UACPI_STATUS_OK;
}

static void uacpi_irq_trampoline(uint8_t isr, uint64_t error, uint64_t vec, void *opaque) {
	uacpi_irq_req *r = (uacpi_irq_req *)opaque;
	if (r != NULL) {
		r->handler(r->ctx);
	}
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_irq_handle) {

	if (!out_irq_handle) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	if (uacpi_deferred_count >= UACPI_DEFER_MAX) {
		return UACPI_STATUS_INTERNAL_ERROR;
	}

	uacpi_irq_req *r = buddy_malloc(&acpi_pool, sizeof(uacpi_irq_req));
	if (r == NULL) {
		return UACPI_STATUS_OUT_OF_MEMORY;
	}

	r->gsi = irq;
	r->handler = handler;
	r->ctx = ctx;
	r->vector = 0;
	r->claimed = false;

	/* If claim phase already started, bind immediately using pci_irq_routes[]. */
	if (uacpi_claim_phase) {
		for (size_t j = 0; j < MAX_PCI_ROUTES; j++) {
			pci_irq_route_t *route = &pci_irq_routes[j];
			if (!route->exists || route->gsi != r->gsi) {
				continue;
			}
			uint8_t vector = (uint8_t)(IRQ_START + (uint32_t)j);
			dprintf("[uACPI] Attached interrupt on GSI %d (ISR %d)\n", r->gsi, vector);
			if (register_interrupt_handler(vector, uacpi_irq_trampoline, (pci_dev_t){0}, r)) {
				r->vector = vector;
				r->claimed = true;
			} else {
				dprintf("acpi: failed to register vector %u for GSI %u\n", vector, route->gsi);
			}
			break;
		}
	} else {
		dprintf("[uACPI] Attached interrupt on GSI %d (deferred)\n", r->gsi);
	}

	uacpi_deferred[uacpi_deferred_count++] = r;

	*out_irq_handle = (uacpi_handle)(uintptr_t)r;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler, uacpi_handle irq_handle) {

	uacpi_irq_req *r = (uacpi_irq_req *)(uintptr_t)irq_handle;
	if (r == NULL) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	for (size_t i = 0; i < uacpi_deferred_count; i++) {
		if (uacpi_deferred[i] == r) {
			if (r->claimed) {
				deregister_interrupt_handler(r->vector, uacpi_irq_trampoline);
			}
			uacpi_deferred[i] = uacpi_deferred[uacpi_deferred_count - 1];
			uacpi_deferred[uacpi_deferred_count - 1] = NULL;
			uacpi_deferred_count--;
			buddy_free(&acpi_pool, r);
			return UACPI_STATUS_OK;
		}
	}

	return UACPI_STATUS_INTERNAL_ERROR;
}

void acpi_claim_deferred_irqs(void) {
	if (uacpi_claim_phase) {
		return;
	}
	for (size_t i = 0; i < uacpi_deferred_count; i++) {
		uacpi_irq_req *r = uacpi_deferred[i];
		if (r == NULL || r->claimed) {
			continue;
		}
		for (size_t j = 0; j < MAX_PCI_ROUTES; j++) {
			pci_irq_route_t *route = &pci_irq_routes[j];
			if (!route->exists || route->gsi != r->gsi) {
				continue;
			}
			uint8_t vector = (uint8_t)(IRQ_START + (uint32_t)j);
			if (register_interrupt_handler(vector, uacpi_irq_trampoline, (pci_dev_t){0}, r)) {
				r->vector = vector;
				r->claimed = true;
			} else {
				dprintf("acpi: failed to register vector %u for GSI %u\n", vector, route->gsi);
			}
			break;
		}
	}
	uacpi_claim_phase = true;
	power_button_init();
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
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx) {
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
	uacpi_resources *resources = NULL;
	if (uacpi_get_current_resources(node, &resources) != UACPI_STATUS_OK || !resources) {
		return UACPI_ITERATION_DECISION_CONTINUE;
	}
	uacpi_for_each_resource(resources, resource_callback, NULL);
	uacpi_free_resources(resources);
	return UACPI_ITERATION_DECISION_CONTINUE;
}

void enumerate_all_gsis(void) {
	uacpi_namespace_for_each_child(uacpi_namespace_root(), device_callback, NULL, UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY, NULL);
}

static uint8_t shutdown_cpus = 0;

void register_shutdown_ap(void) {
	__atomic_add_fetch(&shutdown_cpus, 1, __ATOMIC_SEQ_CST);
}

bool shutdown(void) {
	dprintf("uACPI shutdown process started...\n");
	/* Tell all CPUs to stop. Give them 3 seconds, and then proceed anyway */
	apic_send_ipi(0xFFFFFFFF, APIC_HALT_IPI);
	uint64_t tick_start = get_ticks();
	while (__atomic_load_n(&shutdown_cpus, __ATOMIC_SEQ_CST) < get_cpu_count() - 1 && get_ticks() - tick_start < 3000) {
		_mm_pause();
	}
	uacpi_status ret = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
	if (uacpi_unlikely_error(ret)) {
		dprintf("[uACPI] Failed to prepare for sleep: %s", uacpi_status_to_string(ret));
		return false;
	}
	dprintf("Entering S5 sleep state...\n");
	interrupts_off();
	ret = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
	if (uacpi_unlikely_error(ret)) {
		dprintf("[uACPI] Failed to enter sleep: %s", uacpi_status_to_string(ret));
		return false;
	}
	return true;
}

/*
 * This handler will be called by uACPI from an interrupt context,
 * whenever a power button press is detected.
 */
static uacpi_interrupt_ret handle_power_button(uacpi_handle ctx) {
	dprintf("uACPI power button event triggered\n");
	shutdown();
	return UACPI_INTERRUPT_HANDLED;
}

bool power_button_init(void) {
	uacpi_status ret = uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_POWER_BUTTON, handle_power_button, UACPI_NULL);
	if (uacpi_unlikely_error(ret)) {
		dprintf("[uACPI] Failed to install power button event callback: %s", uacpi_status_to_string(ret));
		return false;
	}
	dprintf("uACPI power button event installed\n");
	return true;
}