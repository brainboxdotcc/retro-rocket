#include <kernel.h>
#include <cpuid.h>

#define IA32_APIC_BASE_MSR 0x1B
#define APIC_BASE_X2APIC_ENABLE (1ULL << 10)

 static uint64_t saved_lapic = 0;

 uint64_t get_lapic_address() {
	if (saved_lapic) {
		return saved_lapic;
	}
	uint32_t eax, edx;
	__asm__ volatile (
		"rdmsr"
		: "=a"(eax), "=d"(edx)
		: "c"(0x1B)  // APIC_BASE_MSR
		);
	uint64_t result = ((uint64_t)edx << 32) | eax;
	result &= 0xFFFFFFFFFFFFF000ULL;
	saved_lapic = result;
	return result;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
	uint32_t lo = (uint32_t)value;
	uint32_t hi = (uint32_t)(value >> 32);
	__asm__ volatile ("wrmsr"
		:
		: "c"(msr), "a"(lo), "d"(hi));
}


void apic_write(uint64_t reg, uint32_t val) {
	if (x2apic_enabled()) {
		wrmsr(0x800 + (reg >> 4), val);
	} else {
		uint64_t lapic_base = get_lapic_address();
		*(volatile uint32_t *)(lapic_base + reg) = val;
	}
}

uint32_t apic_read(uint64_t reg) {
	if (x2apic_enabled()) {
		return (uint32_t)rdmsr(0x800 + (reg >> 4));
	} else {
		uint64_t lapic_base = get_lapic_address();
		return *(volatile uint32_t *)(lapic_base + reg);
	}
}

uint32_t cpu_id(void) {
	if (x2apic_enabled()) {
		// x2APIC: full 32-bit ID from MSR 0x802
		return (uint32_t)rdmsr(0x802);
	} else {
		// xAPIC: 8-bit ID from MMIO register (0x20 >> 24)
		return (apic_read(APIC_ID) >> 24) & 0xFF;
	}
}

uint8_t logical_cpu_id(void) {
	return get_cpu_id_from_lapic_id(cpu_id());
}

uint64_t rdmsr(uint32_t msr) {
	uint32_t lo, hi;
	__asm__ volatile ("rdmsr"
		: "=a"(lo), "=d"(hi)
		: "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

int x2apic_supported(void) {
	unsigned int eax, ebx, ecx, edx;

	if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
		return 0;

	return (ecx & (1u << 21)) != 0; // Bit 21 of ECX = x2APIC support
}

int x2apic_enabled(void) {
	 if (!x2apic_supported()) {
		 return 0; // CPU doesn't even support x2APIC
	 }
	 // Read APIC base MSR to see if x2APIC mode is active
	 uint64_t apic_base = rdmsr(IA32_APIC_BASE_MSR);
	 return (apic_base & APIC_BASE_X2APIC_ENABLE) != 0;
 }
