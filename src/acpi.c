#include <kernel.h>

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

static uint32_t irq_override_map[256];

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
						dprintf("Interrupt override: IRQ %d -> GSI %d (flags: 0x%x)\n", ovr->irq_source, ovr->gsi, ovr->flags);
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
		if (irq_override_map[i] == 0 && i != 0) // leave IRQ0 unmodified in case overridden
			irq_override_map[i] = i;
	}
	if (numcore > 0) {
		kprintf("SMP: %d cores, %d IOAPICs\n", numcore, numioapic);
	}
	for (int i = 0; i < 16; ++i) {
		dprintf("IRQ %d maps to GSI %d\n", i, irq_to_gsi(i));
	}
}

uint32_t irq_to_gsi(uint8_t irq) {
	return irq_override_map[irq];
}

uint8_t get_irq_polarity(uint8_t irq) {
	return irq_polarity[irq];
}

uint8_t get_irq_trigger_mode(uint8_t irq) {
	return irq_trigger_mode[irq];
}