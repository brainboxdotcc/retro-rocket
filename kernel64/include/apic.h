#ifndef __APIC_H__
#define __APIC_H__

#define APIC_ADDRESS 0x4000
#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_ENABLE 0x800

// All of these values are offset from the APIC base address
#define APIC_ID 0x0020
#define APIC_VERSION 0x0030

u32 apic_read(u64 reg);
void apic_write(u64 reg, u32 value);
u8 cpu_id();

#endif
