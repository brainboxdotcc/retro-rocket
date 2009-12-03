#include "../include/kernel.h"
#include "../include/cpuid.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800

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
	read_msr(IA32_APIC_BASE_MSR,&a,&d);
	return (a&0xfffff000)|((u64)(d&0x0f)<<32);
}

void set_apic_base(u64 apicaddr)
{
	u64 a=(apicaddr&0xfffff000) | IA32_APIC_BASE_MSR_ENABLE;
	u32 d=(apicaddr>>32) & 0x0f;
	write_msr(IA32_APIC_BASE_MSR, a,d);
}

int apic_enabled()
{
	u32 a, d;
	read_msr(IA32_APIC_BASE_MSR, &a, &d);
	return ((a & (1 << 11)) >> 11);
}

/* Detect local APIC via CPUID */
int detect_apic()
{
	u32 ecx, edx;
	cpuid(CPUID_GETFEATURES, &ecx, &edx);
	if (edx & CPUID_FEAT_EDX_APIC)
	{
		printf("Detected a%s local APIC, base: %016x\n", apic_enabled() ? "n enabled" : " disabled", get_apic_base());
		// Relocate it to itself as a debug op to check we found it.
		// In bochs, this outputs a message:
		// [APIC0] relocate APIC id=0 to 0xfee00000
		set_apic_base(get_apic_base());
		return 1;
	}
	return 0;
}
