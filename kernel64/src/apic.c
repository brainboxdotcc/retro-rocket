#include "../include/kernel.h"
#include "../include/cpuid.h"

#define APIC_ADDRESS 0x4000
#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_ENABLE 0x800

// All of these values are offset from the APIC base address
#define APIC_ID 0x0020
#define APIC_VERSION 0x0030

volatile u32* APIC;

void read_msr(u32 msr, u32 *lo, u32 *hi)
{
	asm volatile("rdmsr":"=a"(*lo),"=d"(*hi):"c"(msr));
}

void write_msr(u32 msr, u32 lo, u32 hi)
{
	asm volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr));
}

u64 get_apic_base()
{
	u32 a,d;
	read_msr(APIC_BASE_MSR,&a,&d);
	return (a&0xfffff000)|((u64)(d&0x0f)<<32);
}

void set_apic_base(u64 apicaddr)
{
	memset((void*)apicaddr, 0xff, 4096);
	u64 a=(apicaddr&0xfffff000) | APIC_BASE_MSR_ENABLE;
	u32 d=(apicaddr>>32) & 0x0f;
	write_msr(APIC_BASE_MSR, a, d);
	APIC = apicaddr;
}

int apic_enabled()
{
	u32 a, d;
	read_msr(APIC_BASE_MSR, &a, &d);
	return ((a & (1 << 11)) >> 11);
}

u32 apic_read(u64 reg)
{
	return *((volatile u32 *)(APIC + reg));
}

void apic_write(u64 reg, u32 value)
{
	*((volatile u32 *)(APIC + reg)) = value;
}


/* Detect local APIC via CPUID */
int detect_apic()
{
	u32 ecx, edx;
	cpuid(CPUID_GETFEATURES, &ecx, &edx);
	if (edx & CPUID_FEAT_EDX_APIC)
	{
		set_apic_base(APIC_ADDRESS);
		printf("Detected a%s local APIC, base: %08x\n", apic_enabled() ? "n enabled" : " disabled", get_apic_base());
		printf("Local APIC version: %d ID: %d\n", apic_read(APIC_VERSION) & 0xFF, ((apic_read(APIC_ID))>>24)&0xFF);
		return 1;
	}
	return 0;
}
