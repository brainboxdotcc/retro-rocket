#include "../include/kernel.h"
#include "../include/cpuid.h"

/* Detect local APIC via CPUID */
int detect_apic()
{
	u32 ecx, edx;
	cpuid(CPUID_GETFEATURES, &ecx, &edx);
	return (edx & CPUID_FEAT_EDX_APIC);
}
