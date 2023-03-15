#include <kernel.h>

volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

uint8_t lapic_ids[256] = { 0 }; // CPU core Local APIC IDs
uint8_t ioapic_ids[256] = { 0 }; // CPU core Local APIC IDs
uint16_t numcore = 0;         // number of cores detected
uint16_t numioapic = 0;
uint64_t lapic_ptr = 0;       // pointer to the Local APIC MMIO registers
uint64_t ioapic_ptr[256] = { 0 };      // pointer to the IO APIC MMIO registers
uint32_t ioapic_gsi_base[256] = { 0 };
uint8_t ioapic_gsi_count[256] = { 0 };

rsdp_t* get_rsdp() {
	return (rsdp_t*)rsdp_request.response->address;
}

sdt_header_t* get_sdt_header() {
	rsdp_t* rsdp = get_rsdp();
	return (sdt_header_t*)(uint64_t)rsdp->rsdt_address;
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
	return (uint64_t)lapic_ptr;
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

void detect_cores()
{
	uint8_t *ptr, *ptr2;
	uint32_t len;
	uint8_t* rsdt = (uint8_t*)get_sdt_header();
	numcore = 0;
	numioapic = 0;

	// Iterate on ACPI table pointers
	for(len = *((uint32_t*)(rsdt + 4)), ptr2 = rsdt + 36; ptr2 < rsdt + len; ptr2 += rsdt[0]=='X' ? 8 : 4) {
		ptr = (uint8_t*)(uint64_t)(rsdt[0]=='X' ? *((uint64_t*)ptr2) : *((uint32_t*)ptr2));
		if(*ptr == 'A' && *(ptr + 1) == 'P' && *(ptr + 2) == 'I' && *(ptr + 3) == 'C') {
			// found MADT
			lapic_ptr = (uint64_t)(*((uint32_t*)(ptr+0x24)));
			kprintf("Detected: 32-Bit Local APIC [base: %016x]\n", lapic_ptr);
			ptr2 = ptr + *((uint32_t*)(ptr + 4));
			// iterate on variable length records
			for(ptr += 44; ptr < ptr2; ptr += ptr[1]) {
				switch(ptr[0]) {
					case 0:
						if(ptr[4] & 1) {
							lapic_ids[numcore++] = ptr[3];
						}
					break; // found Processor Local APIC
					case 1:
						ioapic_ptr[numioapic] = (uint64_t)*((uint32_t*)(ptr+4));
						ioapic_ids[numioapic] = ptr[2];
						ioapic_gsi_base[numioapic] = (uint32_t)*((uint32_t*)(ptr+8));
						uint32_t *mmio = (uint32_t*)ioapic_ptr[numioapic];
						mmio[0] = 0x01;
						uint32_t count = ((mmio[0x10 / 4]) & 0xFF) + 1;
						ioapic_gsi_count[numioapic] = count;
						kprintf("Detected: IOAPIC [base: %016x; id: %d gsi base: %d gsi count: %d]\n", ioapic_ptr[numioapic], ioapic_ids[numioapic], ioapic_gsi_base[numioapic], count);
						numioapic++;
					break;  // found IOAPIC
					case 5:
						lapic_ptr = *((uint64_t*)(ptr+4));
						kprintf("Detected: 64-Bit Local APIC [base: %016x]\n", lapic_ptr);
					break;             // found 64 bit LAPIC
				}
			}
			break;
		}
	}
	if (numcore > 0) {
		kprintf("SMP: %d cores, %d IOAPICs\n", numcore, numioapic);
	}
}
