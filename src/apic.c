 #include <kernel.h>

void get_lapic_address(uint64_t* lapic);

uint32_t apic_read(uint64_t reg)
{
	uint64_t lapic = 0;
	get_lapic_address(&lapic);
	uint32_t res = *((volatile uint32_t *)(lapic + reg));
	return res;
}

void apic_write(uint64_t reg, uint32_t value)
{
	uint64_t lapic = 0;
	get_lapic_address(&lapic);
	*((volatile uint32_t *)(lapic + reg)) = value;
}

uint8_t cpu_id()
{
	uint32_t id = apic_read(APIC_ID);
	id = (id >> 24) & 0xff;
	return id;
}
