 #include <kernel.h>

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

uint32_t apic_read(uint64_t reg)
{
	uint64_t lapic = get_lapic_address();
	uint32_t res = *((volatile uint32_t *)(lapic + reg));
	return res;
}

void apic_write(uint64_t reg, uint32_t value)
{
	uint64_t lapic = get_lapic_address();
	*((volatile uint32_t *)(lapic + reg)) = value;
}

uint8_t cpu_id()
{
	uint32_t id = apic_read(APIC_ID);
	id = (id >> 24) & 0xff;
	return id;
}
