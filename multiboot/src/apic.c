#include <kernel.h>
#include <cpuid.h>

#define APIC_ADDRESS 0x4000
#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_ENABLE 0x800

// All of these values are offset from the APIC base address
#define APIC_ID 0x0020
#define APIC_VERSION 0x0030

volatile u32int* APIC;

void read_msr(u32int msr, u32int *lo, u32int *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void write_msr(u32int msr, u32int lo, u32int hi)
{
	asm volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}

u32int get_apic_base()
{
	u32int a, d;
	read_msr(APIC_BASE_MSR, &a, &d);
	return (a & 0xfffff000);
}

void set_apic_base(u32int apicaddr)
{
	u32int a = (apicaddr & 0xfffff000) | APIC_BASE_MSR_ENABLE;
	u32int d = (apicaddr >> 32) & 0x0f;
	write_msr(APIC_BASE_MSR, a, d);
	APIC = apicaddr;
}

int apic_enabled()
{
	u32int a, d;
	read_msr(APIC_BASE_MSR, &a, &d);
	return ((a & (1 << 11)) >> 11);
}

u32int apic_read(u32int reg)
{
	return *((volatile u32int *)(APIC + reg));
}

void apic_write(u32int reg, u32int value)
{
	*((volatile u32int *)(APIC + reg)) = value;
}


/* Detect local APIC via CPUID */
int detect_apic()
{
	u32int ecx, edx;
	cpuid(CPUID_GETFEATURES, &ecx, &edx);
	if (edx & CPUID_FEAT_EDX_APIC)
	{
		set_apic_base(APIC_ADDRESS);
		kprintf("Detected a%s local APIC, base: %08x\n", apic_enabled() ? "n enabled" : " disabled", get_apic_base());
		kprintf("Local APIC version: %d ID: %d\n", apic_read(APIC_VERSION) & 0xFF, ((apic_read(APIC_ID)) >> 24) & 0xFF);
		DumpHex(APIC, 0x100);
		return 1;
	}
	return 0;
}

