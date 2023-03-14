 #include <kernel.h>

 void get_lapic_address(u64* lapic);

u32 apic_read(u64 reg)
{
	u64 lapic = 0;
	get_lapic_address(&lapic);
	u32 res = *((volatile u32 *)(lapic + reg));
	return res;
}

void apic_write(u64 reg, u32 value)
{
	u64 lapic = 0;
	get_lapic_address(&lapic);
	*((volatile u32 *)(lapic + reg)) = value;
}

u8 cpu_id()
{
	u32 id = apic_read(APIC_ID);
	id = (id >> 24) & 0xff;
	return id;
}
