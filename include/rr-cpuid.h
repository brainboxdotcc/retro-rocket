/**
 * @file cpuid.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#ifndef __CPUID_H__
#define __CPUID_H__

#include <stdint.h>

enum cpuid_flags : uint32_t
{
	CPUID_FEAT_ECX_SSE3		= 1 << 0, 
	CPUID_FEAT_ECX_PCLMUL		= 1 << 1,
	CPUID_FEAT_ECX_DTES64		= 1 << 2,
	CPUID_FEAT_ECX_MONITOR		= 1 << 3,	
	CPUID_FEAT_ECX_DS_CPL		= 1 << 4,	
	CPUID_FEAT_ECX_VMX		= 1 << 5,	
	CPUID_FEAT_ECX_SMX		= 1 << 6,	
	CPUID_FEAT_ECX_EST		= 1 << 7,	
	CPUID_FEAT_ECX_TM2		= 1 << 8,	
	CPUID_FEAT_ECX_SSSE3		= 1 << 9,	
	CPUID_FEAT_ECX_CID		= 1 << 10,
	CPUID_FEAT_ECX_FMA		= 1 << 12,
	CPUID_FEAT_ECX_CX16		= 1 << 13, 
	CPUID_FEAT_ECX_ETPRD		= 1 << 14, 
	CPUID_FEAT_ECX_PDCM		= 1 << 15, 
	CPUID_FEAT_ECX_DCA		= 1 << 18, 
	CPUID_FEAT_ECX_SSE4_1		= 1 << 19, 
	CPUID_FEAT_ECX_SSE4_2		= 1 << 20, 
	CPUID_FEAT_ECX_x2APIC		= 1 << 21, 
	CPUID_FEAT_ECX_MOVBE		= 1 << 22, 
	CPUID_FEAT_ECX_POPCNT		= 1 << 23, 
	CPUID_FEAT_ECX_XSAVE		= 1 << 26, 
	CPUID_FEAT_ECX_OSXSAVE		= 1 << 27, 
	CPUID_FEAT_ECX_AVX		= 1 << 28,
 
	CPUID_FEAT_EDX_FPU		= 1 << 0,	
	CPUID_FEAT_EDX_VME		= 1 << 1,	
	CPUID_FEAT_EDX_DE		= 1 << 2,	
	CPUID_FEAT_EDX_PSE		= 1 << 3,	
	CPUID_FEAT_EDX_TSC		= 1 << 4,	
	CPUID_FEAT_EDX_MSR		= 1 << 5,	
	CPUID_FEAT_EDX_PAE		= 1 << 6,	
	CPUID_FEAT_EDX_MCE		= 1 << 7,	
	CPUID_FEAT_EDX_CX8		= 1 << 8,	
	CPUID_FEAT_EDX_APIC		= 1 << 9,	
	CPUID_FEAT_EDX_SEP		= 1 << 11, 
	CPUID_FEAT_EDX_MTRR		= 1 << 12, 
	CPUID_FEAT_EDX_PGE		= 1 << 13, 
	CPUID_FEAT_EDX_MCA		= 1 << 14, 
	CPUID_FEAT_EDX_CMOV		= 1 << 15, 
	CPUID_FEAT_EDX_PAT		= 1 << 16, 
	CPUID_FEAT_EDX_PSE36		= 1 << 17, 
	CPUID_FEAT_EDX_PSN		= 1 << 18, 
	CPUID_FEAT_EDX_CLF		= 1 << 19, 
	CPUID_FEAT_EDX_DTES		= 1 << 21, 
	CPUID_FEAT_EDX_ACPI		= 1 << 22, 
	CPUID_FEAT_EDX_MMX		= 1 << 23, 
	CPUID_FEAT_EDX_FXSR		= 1 << 24, 
	CPUID_FEAT_EDX_SSE		= 1 << 25, 
	CPUID_FEAT_EDX_SSE2		= 1 << 26, 
	CPUID_FEAT_EDX_SS		= 1 << 27, 
	CPUID_FEAT_EDX_HTT		= 1 << 28, 
	CPUID_FEAT_EDX_TM1		= 1 << 29, 
	CPUID_FEAT_EDX_IA64		= 1 << 30,
	CPUID_FEAT_EDX_PBE		= (uint32_t)(1 << 31)
};

enum cpuid_requests : uint32_t
{
	CPUID_GETVENDORSTRING=0x00000000,
	CPUID_GETFEATURES,
	CPUID_GETTLB,
	CPUID_GETSERIAL, 
	CPUID_INTELEXTENDED=0x80000000,
	CPUID_INTELFEATURES,
	CPUID_INTELBRANDSTRING,
	CPUID_INTELBRANDSTRINGMORE,
	CPUID_INTELBRANDSTRINGEND,
};


static inline void cpuid(int code, uint32_t *a, uint32_t *d)
{
	asm volatile("cpuid":"=a"(*a),"=d"(*d):"0"(code):"ecx","ebx");
}

static inline int cpuid_string(int code, uint32_t where[4])
{
	int highest;
	asm volatile("cpuid":"=a"(*where),"=b"(*(where+1)),
		"=c"(*(where+2)),"=d"(*(where+3)):"0"(code));
	return highest;
}

#endif
