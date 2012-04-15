 #include <kernel.h>

u32 apic_read(u64 reg)
{
	return *((volatile u32 *)(hydrogen_info->lapic_paddr + reg));
}

void apic_write(u64 reg, u32 value)
{
	*((volatile u32 *)(hydrogen_info->lapic_paddr + reg)) = value;
}

u8 cpu_id()
{
	u32 id = apic_read(APIC_ID);
	id = (id >> 24) & 0xff;
	return id;
}
