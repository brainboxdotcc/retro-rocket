#include <kernel.h>
#include <mmio.h>
#include <uacpi/uacpi.h>

volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

static uint8_t lapic_ids[256] = { 0 }; // CPU core Local APIC IDs
static uint8_t ioapic_ids[256] = { 0 }; // CPU core Local APIC IDs
static uint16_t numcore = 0;         // number of cores detected
static uint16_t numioapic = 0;
static uint64_t lapic_ptr = 0;       // pointer to the Local APIC MMIO registers
static uint64_t ioapic_ptr[256] = { 0 };      // pointer to the IO APIC MMIO registers
static uint32_t ioapic_gsi_base[256] = { 0 };
static uint8_t ioapic_gsi_count[256] = { 0 };
static uint8_t irq_trigger_mode[256] = {0};  // 0 = edge, 1 = level
static uint8_t irq_polarity[256]     = {0};  // 0 = high,  1 = low

static uint32_t irq_override_map[256] = {
	[0 ... 255] = 0xFFFFFFFF
};

rsdp_t* get_rsdp() {
	return (rsdp_t*) rsdp_request.response->address;
}

sdt_header_t* get_sdt_header() {
	rsdp_t* rsdp = get_rsdp();
	return (sdt_header_t*) (uint64_t) rsdp->rsdt_address;
}

uint8_t* get_lapic_ids()
{
	return lapic_ids;
}

uint16_t get_cpu_count()
{
	return numcore;
}

uint64_t get_local_apic()
{
	return (uint64_t) lapic_ptr;
}

uint16_t get_ioapic_count()
{
	return numioapic;
}

ioapic_t get_ioapic(uint16_t index)
{
	ioapic_t ioapic = { 0 };
	if (index >= numioapic) {
		return ioapic;
	}
	ioapic.gsi_base = ioapic_gsi_base[index];
	ioapic.gsi_count = ioapic_gsi_count[index];
	ioapic.id = ioapic_ids[index];
	ioapic.paddr = ioapic_ptr[index];
	return ioapic;
}

void init_cores()
{
	uint8_t *ptr, *ptr2;
	uint32_t len;
	uint8_t* rsdt = (uint8_t*)get_sdt_header();
	numcore = 0;
	numioapic = 0;

	// Iterate on ACPI table pointers
	for (len = *((uint32_t*)(rsdt + 4)), ptr2 = rsdt + 36; ptr2 < rsdt + len; ptr2 += rsdt[0]=='X' ? 8 : 4) {
		ptr = (uint8_t*)(uint64_t)(rsdt[0]=='X' ? *((uint64_t*)ptr2) : *((uint32_t*)ptr2));
		if (*ptr == 'A' && *(ptr + 1) == 'P' && *(ptr + 2) == 'I' && *(ptr + 3) == 'C') {
			// found MADT
			lapic_ptr = (uint64_t)(*((uint32_t*)(ptr + 0x24)));
			kprintf("Detected: 32-Bit Local APIC [base: %llx]\n", lapic_ptr);
			ptr2 = ptr + *((uint32_t*)(ptr + 4));
			// iterate on variable length records
			for (ptr += 44; ptr < ptr2; ptr += ptr[1]) {
				switch (ptr[0]) {
					case 0:
						if (ptr[4] & 1) {
							lapic_ids[numcore++] = ptr[3];
						}
					break; // found Processor Local APIC
					case 1:
						ioapic_ptr[numioapic] = (uint64_t)*((uint32_t*)(ptr + 4));
						ioapic_ids[numioapic] = ptr[2];
						ioapic_gsi_base[numioapic] = (uint32_t)*((uint32_t*)(ptr + 8));
						uint32_t *mmio = (uint32_t*)ioapic_ptr[numioapic];
						mmio[0] = 0x01;
						uint32_t count = ((mmio[0x10 / 4]) & 0xFF) + 1;
						ioapic_gsi_count[numioapic] = count;
						kprintf("Detected: IOAPIC [base: %llx; id: %d gsi base: %d gsi count: %d]\n", ioapic_ptr[numioapic], ioapic_ids[numioapic], ioapic_gsi_base[numioapic], count);
						numioapic++;
					break;  // found IOAPIC
					case 2: {
						madt_override_t* ovr = (madt_override_t*)ptr;
						kprintf("Interrupt override: IRQ %d -> GSI %d (flags: 0x%x)\n", ovr->irq_source, ovr->gsi, ovr->flags);
						irq_override_map[ovr->irq_source] = ovr->gsi;
						if (ovr->flags & 0x8) irq_trigger_mode[ovr->irq_source] = 1; // Bit 3 = trigger mode
						if (ovr->flags & 0x2) irq_polarity[ovr->irq_source]     = 1; // Bit 1 = polarity
						break;
					}
					case 5:
						lapic_ptr = *((uint64_t*)(ptr + 4));
						kprintf("Detected: 64-Bit Local APIC [base: %llx]\n", lapic_ptr);
					break;             // found 64 bit LAPIC
				}
			}
			break;
		}
	}
	for (int i = 0; i < 16; ++i) {
		if (irq_override_map[i] == 0xFFFFFFFF) {
			// leave IRQ0 unmodified in case overridden
			irq_override_map[i] = i;
		}
	}
	if (numcore > 0) {
		kprintf("SMP: %d cores, %d IOAPICs\n", numcore, numioapic);
	}
	for (int i = 0; i < 16; ++i) {
		dprintf("IRQ %d maps to GSI %d\n", i, irq_to_gsi(i));
	}
}

uint32_t irq_to_gsi(uint8_t irq) {
	if (irq_override_map[irq] == 0xFFFFFFFF) {
		return irq; // fallback: identity mapping
	}
	return irq_override_map[irq];
}

uint8_t get_irq_polarity(uint8_t irq) {
	return (irq_override_map[irq] == 0xFFFFFFFF)
	       ? IRQ_DEFAULT_POLARITY
	       : irq_polarity[irq];
}

uint8_t get_irq_trigger_mode(uint8_t irq) {
	return (irq_override_map[irq] == 0xFFFFFFFF)
	       ? IRQ_DEFAULT_TRIGGER
	       : irq_trigger_mode[irq];
}

typedef struct {
	pci_dev_t dev;
} uacpi_pci_dev_wrapper_t;

// Logging
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {
	dprintf("[uACPI] %s\n", msg);
}

// Memory allocation
void *uacpi_kernel_alloc(uacpi_size size) {
	return kmalloc(size);
}

void uacpi_kernel_free(void *ptr) {
	kfree(ptr);
}

// Physical memory mapping (flat model)
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
	return (void *)(uintptr_t)addr; // identity-mapped
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
	// no-op in flat memory
}

void uacpi_kernel_stall(uacpi_u8 usec) {

}

void uacpi_kernel_sleep(uacpi_u64 msec) {

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
	return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) { }

uacpi_status uacpi_kernel_io_read8(uacpi_handle, uacpi_size offset, uacpi_u8 *out_value) {
	*out_value = mmio_read8(offset);
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_read16(uacpi_handle, uacpi_size offset, uacpi_u16 *out_value) {
	*out_value = mmio_read16(offset);
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_read32(uacpi_handle, uacpi_size offset, uacpi_u32 *out_value) {
	*out_value = mmio_read32(offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle, uacpi_size offset, uacpi_u8 in_value) {
	mmio_write8(offset, in_value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle, uacpi_size offset, uacpi_u16 in_value) {
	mmio_write16(offset, in_value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle, uacpi_size offset, uacpi_u32 in_value) {
	mmio_write32(offset, in_value);
	return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
	uacpi_kernel_free(handle);
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	*value = (uacpi_u8)(pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	*value = (uacpi_u16)(pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	*value = (uacpi_u32)(pci_read(wrapper->dev, offset));
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 value) {
	uacpi_pci_dev_wrapper_t *wrapper = (uacpi_pci_dev_wrapper_t *)handle;
	pci_write(wrapper->dev, offset, value);
	return UACPI_STATUS_OK;
}

// Stub mutex/event/threading
uacpi_handle uacpi_kernel_create_mutex(void) { return (uacpi_handle)1; }
void uacpi_kernel_free_mutex(uacpi_handle h) {}
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16) { return UACPI_STATUS_OK; }
void uacpi_kernel_release_mutex(uacpi_handle) {}

uacpi_handle uacpi_kernel_create_event(void) { return (uacpi_handle)1; }
void uacpi_kernel_free_event(uacpi_handle h) {}
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) { return true; }
void uacpi_kernel_signal_event(uacpi_handle) {}
void uacpi_kernel_reset_event(uacpi_handle) {}

uacpi_thread_id uacpi_kernel_get_thread_id(void) { return 1; }

// Optional no-op spinlock support
uacpi_handle uacpi_kernel_create_spinlock(void) { return (uacpi_handle)1; }
void uacpi_kernel_free_spinlock(uacpi_handle) {}
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle) { return 0; }
void uacpi_kernel_unlock_spinlock(uacpi_handle, uacpi_cpu_flags) {}

// RSDP (you already have this)
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
	*out_rsdp_address = rsdp_request.response->address;
	return UACPI_STATUS_OK;
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) { return 0; };

uacpi_status uacpi_kernel_install_interrupt_handler(
	uacpi_u32 irq,
	uacpi_interrupt_handler handler,
	uacpi_handle ctx,
	uacpi_handle *out_irq_handle
) {
	(void)irq;
	(void)handler;
	(void)ctx;
	static int dummy;
	*out_irq_handle = &dummy;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
	uacpi_interrupt_handler handler,
	uacpi_handle irq_handle
) {
	(void)handler;
	(void)irq_handle;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
	dprintf("uACPI firmware request: type=%u\n", req->type);
	return UACPI_STATUS_OK;
}

void async_run_gpe_handler(uacpi_handle gpe) {
	(void)gpe;
	// No GPE support yet
}

uacpi_status uacpi_kernel_schedule_work(
	uacpi_work_type type,
	uacpi_work_handler handler,
	uacpi_handle ctx
) {
	(void)type;
	(void)ctx;

	// Synchronously call it immediately (not technically correct, but fine for now)
	handler(ctx);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
	// No deferred work in Retro Rocket yet
	return UACPI_STATUS_OK;
}

