/**
 * @file cpuid.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cpuid.h>

/**
 * @brief CPUID feature flags returned in leaf 1 ECX and EDX
 */
enum cpuid_flags
{
	/** @brief Streaming SIMD Extensions 3 support */
	CPUID_FEAT_ECX_SSE3		= 1 << 0,
	/** @brief Carry-less multiplication instruction support */
	CPUID_FEAT_ECX_PCLMUL		= 1 << 1,
	/** @brief 64-bit debug store area support */
	CPUID_FEAT_ECX_DTES64		= 1 << 2,
	/** @brief MONITOR and MWAIT instruction support */
	CPUID_FEAT_ECX_MONITOR		= 1 << 3,
	/** @brief CPL qualified debug store support */
	CPUID_FEAT_ECX_DS_CPL		= 1 << 4,
	/** @brief Virtual Machine Extensions support */
	CPUID_FEAT_ECX_VMX		= 1 << 5,
	/** @brief Safer Mode Extensions support */
	CPUID_FEAT_ECX_SMX		= 1 << 6,
	/** @brief Enhanced SpeedStep support */
	CPUID_FEAT_ECX_EST		= 1 << 7,
	/** @brief Thermal Monitor 2 support */
	CPUID_FEAT_ECX_TM2		= 1 << 8,
	/** @brief Supplemental Streaming SIMD Extensions 3 support */
	CPUID_FEAT_ECX_SSSE3		= 1 << 9,
	/** @brief Context ID support */
	CPUID_FEAT_ECX_CID		= 1 << 10,
	/** @brief Fused multiply-add instruction support */
	CPUID_FEAT_ECX_FMA		= 1 << 12,
	/** @brief CMPXCHG16B instruction support */
	CPUID_FEAT_ECX_CX16		= 1 << 13,
	/** @brief Extended task priority messages disable support */
	CPUID_FEAT_ECX_ETPRD		= 1 << 14,
	/** @brief Performance and debug capability MSR support */
	CPUID_FEAT_ECX_PDCM		= 1 << 15,
	/** @brief Direct cache access support */
	CPUID_FEAT_ECX_DCA		= 1 << 18,
	/** @brief Streaming SIMD Extensions 4.1 support */
	CPUID_FEAT_ECX_SSE4_1		= 1 << 19,
	/** @brief Streaming SIMD Extensions 4.2 support */
	CPUID_FEAT_ECX_SSE4_2		= 1 << 20,
	/** @brief x2APIC support */
	CPUID_FEAT_ECX_x2APIC		= 1 << 21,
	/** @brief MOVBE instruction support */
	CPUID_FEAT_ECX_MOVBE		= 1 << 22,
	/** @brief POPCNT instruction support */
	CPUID_FEAT_ECX_POPCNT		= 1 << 23,
	/** @brief XSAVE and related extended state management support */
	CPUID_FEAT_ECX_XSAVE		= 1 << 26,
	/** @brief OSXSAVE support indicating OS-managed XSAVE state */
	CPUID_FEAT_ECX_OSXSAVE		= 1 << 27,
	/** @brief AVX instruction support */
	CPUID_FEAT_ECX_AVX		= 1 << 28,
	/** @brief Hypervisor present flag */
	CPUID_FEAT_ECX_HYPERVISOR	= 1 << 31,

	/** @brief On-chip x87 floating point unit support */
	CPUID_FEAT_EDX_FPU		= 1 << 0,
	/** @brief Virtual 8086 mode extensions support */
	CPUID_FEAT_EDX_VME		= 1 << 1,
	/** @brief Debugging extensions support */
	CPUID_FEAT_EDX_DE		= 1 << 2,
	/** @brief Page size extension support */
	CPUID_FEAT_EDX_PSE		= 1 << 3,
	/** @brief Time stamp counter support */
	CPUID_FEAT_EDX_TSC		= 1 << 4,
	/** @brief Model specific register support */
	CPUID_FEAT_EDX_MSR		= 1 << 5,
	/** @brief Physical address extension support */
	CPUID_FEAT_EDX_PAE		= 1 << 6,
	/** @brief Machine check exception support */
	CPUID_FEAT_EDX_MCE		= 1 << 7,
	/** @brief CMPXCHG8B instruction support */
	CPUID_FEAT_EDX_CX8		= 1 << 8,
	/** @brief Local APIC support */
	CPUID_FEAT_EDX_APIC		= 1 << 9,
	/** @brief SYSENTER and SYSEXIT support */
	CPUID_FEAT_EDX_SEP		= 1 << 11,
	/** @brief Memory type range registers support */
	CPUID_FEAT_EDX_MTRR		= 1 << 12,
	/** @brief Page global enable support */
	CPUID_FEAT_EDX_PGE		= 1 << 13,
	/** @brief Machine check architecture support */
	CPUID_FEAT_EDX_MCA		= 1 << 14,
	/** @brief Conditional move instruction support */
	CPUID_FEAT_EDX_CMOV		= 1 << 15,
	/** @brief Page attribute table support */
	CPUID_FEAT_EDX_PAT		= 1 << 16,
	/** @brief 36-bit page size extension support */
	CPUID_FEAT_EDX_PSE36		= 1 << 17,
	/** @brief Processor serial number support */
	CPUID_FEAT_EDX_PSN		= 1 << 18,
	/** @brief CLFLUSH instruction support */
	CPUID_FEAT_EDX_CLF		= 1 << 19,
	/** @brief Debug store support */
	CPUID_FEAT_EDX_DTES		= 1 << 21,
	/** @brief Thermal monitor and clock control support */
	CPUID_FEAT_EDX_ACPI		= 1 << 22,
	/** @brief MMX instruction support */
	CPUID_FEAT_EDX_MMX		= 1 << 23,
	/** @brief FXSAVE and FXRSTOR support */
	CPUID_FEAT_EDX_FXSR		= 1 << 24,
	/** @brief Streaming SIMD Extensions support */
	CPUID_FEAT_EDX_SSE		= 1 << 25,
	/** @brief Streaming SIMD Extensions 2 support */
	CPUID_FEAT_EDX_SSE2		= 1 << 26,
	/** @brief Self-snoop support */
	CPUID_FEAT_EDX_SS		= 1 << 27,
	/** @brief Hyper-threading technology support */
	CPUID_FEAT_EDX_HTT		= 1 << 28,
	/** @brief Thermal Monitor 1 support */
	CPUID_FEAT_EDX_TM1		= 1 << 29,
	/** @brief IA-64 support */
	CPUID_FEAT_EDX_IA64		= 1 << 30,
	/** @brief Pending break enable support */
	CPUID_FEAT_EDX_PBE		= 1 << 31,
};

/**
 * @brief CPUID leaf 7 subleaf 0 EBX feature flags
 */
enum cpuid_leaf7_ebx_flags
{
	/** @brief FSGSBASE instruction support */
	CPUID_FEAT_7_EBX_FSGSBASE	= 1 << 0,
	/** @brief Supervisor Mode Execution Prevention support */
	CPUID_FEAT_7_EBX_SMEP		= 1 << 7,
	/** @brief Enhanced REP MOVSB and STOSB support */
	CPUID_FEAT_7_EBX_ERMS		= 1 << 9,
	/** @brief INVPCID instruction support */
	CPUID_FEAT_7_EBX_INVPCID	= 1 << 10,
	/** @brief RDSEED instruction support */
	CPUID_FEAT_7_EBX_RDSEED		= 1 << 18,
	/** @brief ADCX and ADOX instruction support */
	CPUID_FEAT_7_EBX_ADX		= 1 << 19,
	/** @brief Supervisor Mode Access Prevention support */
	CPUID_FEAT_7_EBX_SMAP		= 1 << 20,
	/** @brief CLFLUSHOPT instruction support */
	CPUID_FEAT_7_EBX_CLFLUSHOPT	= 1 << 23,
	/** @brief CLWB instruction support */
	CPUID_FEAT_7_EBX_CLWB		= 1 << 24,
	/** @brief SHA instruction extensions support */
	CPUID_FEAT_7_EBX_SHA		= 1 << 29
};

/**
 * @brief CPUID leaf 7 subleaf 0 ECX feature flags
 */
enum cpuid_leaf7_ecx_flags
{
	/** @brief User-mode instruction prevention support */
	CPUID_FEAT_7_ECX_UMIP		= 1 << 2,
	/** @brief Protection keys for user pages support */
	CPUID_FEAT_7_ECX_PKU		= 1 << 3,
	/** @brief OS-managed protection keys support */
	CPUID_FEAT_7_ECX_OSPKE		= 1 << 4,
	/** @brief WAITPKG instruction support */
	CPUID_FEAT_7_ECX_WAITPKG	= 1 << 5,
	/** @brief CET shadow stack support */
	CPUID_FEAT_7_ECX_CET_SS		= 1 << 7,
	/** @brief 5-level paging support */
	CPUID_FEAT_7_ECX_LA57		= 1 << 16,
	/** @brief RDPID instruction support */
	CPUID_FEAT_7_ECX_RDPID		= 1 << 22,
	/** @brief MOVDIRI instruction support */
	CPUID_FEAT_7_ECX_MOVDIRI	= 1 << 27,
	/** @brief MOVDIR64B instruction support */
	CPUID_FEAT_7_ECX_MOVDIR64B	= 1 << 28
};

/**
 * @brief CPUID leaf 7 subleaf 0 EDX feature flags
 */
enum cpuid_leaf7_edx_flags
{
	/** @brief SERIALIZE instruction support */
	CPUID_FEAT_7_EDX_SERIALIZE	= 1 << 14,
	/** @brief CET indirect branch tracking support */
	CPUID_FEAT_7_EDX_IBT		= 1 << 20
};

/**
 * @brief CPUID leaf 7 subleaf 1 EAX feature flags
 */
enum cpuid_leaf7_subleaf1_eax_flags
{
	/** @brief Flexible Return and Event Delivery support */
	CPUID_FEAT_7_1_EAX_FRED		= 1 << 17,
	/** @brief LKGS instruction support */
	CPUID_FEAT_7_1_EAX_LKGS		= 1 << 18,
	/** @brief Hardware history reset support */
	CPUID_FEAT_7_1_EAX_HRESET	= 1 << 22
};

/**
 * @brief CPUID request leaves used by the kernel
 */
enum cpuid_requests
{
	/** @brief Basic CPUID vendor string leaf */
	CPUID_GETVENDORSTRING = 0x00000000,
	/** @brief Basic processor feature information leaf */
	CPUID_GETFEATURES,
	/** @brief TLB and cache descriptor leaf */
	CPUID_GETTLB,
	/** @brief Processor serial number leaf */
	CPUID_GETSERIAL,
	/** @brief Start of extended CPUID leaves */
	CPUID_INTELEXTENDED = 0x80000000,
	/** @brief Extended processor feature information leaf */
	CPUID_INTELFEATURES,
	/** @brief First processor brand string leaf */
	CPUID_INTELBRANDSTRING,
	/** @brief Second processor brand string leaf */
	CPUID_INTELBRANDSTRINGMORE,
	/** @brief Third processor brand string leaf */
	CPUID_INTELBRANDSTRINGEND,
};

/**
 * @brief Cached CPU capability and identification data
 */
typedef struct cpu_caps {
	/** @brief CPU vendor string */
	char vendor[13];
	/** @brief CPU brand string */
	char brand[49];
	/** @brief True if a hypervisor is present */
	bool hypervisor_present;
	/** @brief Hypervisor vendor string */
	char hypervisor_vendor[13];

	/** @brief Highest supported basic CPUID leaf */
	uint32_t max_basic_leaf;
	/** @brief Highest supported extended CPUID leaf */
	uint32_t max_extended_leaf;

	/** @brief Enhanced REP MOVSB and STOSB support */
	bool erms;
	/** @brief FSGSBASE instruction support */
	bool fsgsbase;
	/** @brief Supervisor Mode Execution Prevention support */
	bool smep;
	/** @brief INVPCID instruction support */
	bool invpcid;
	/** @brief RDSEED instruction support */
	bool rdseed;
	/** @brief ADCX and ADOX instruction support */
	bool adx;
	/** @brief Supervisor Mode Access Prevention support */
	bool smap;
	/** @brief CLFLUSHOPT instruction support */
	bool clflushopt;
	/** @brief CLWB instruction support */
	bool clwb;
	/** @brief SHA instruction extensions support */
	bool sha;
	/** @brief User-mode instruction prevention support */
	bool umip;
	/** @brief Protection keys for user pages support */
	bool pku;
	/** @brief OS-managed protection keys support */
	bool ospke;
	/** @brief WAITPKG instruction support */
	bool waitpkg;
	/** @brief RDPID instruction support */
	bool rdpid;
	/** @brief 5-level paging support */
	bool la57;
	/** @brief SERIALIZE instruction support */
	bool serialize;
	/** @brief MOVDIRI instruction support */
	bool movdiri;
	/** @brief MOVDIR64B instruction support */
	bool movdir64b;
	/** @brief CET shadow stack support */
	bool cet_ss;
	/** @brief CET indirect branch tracking support */
	bool ibt;
	/** @brief Flexible Return and Event Delivery support */
	bool fred;
	/** @brief LKGS instruction support */
	bool lkgs;
	/** @brief Hardware history reset support */
	bool hreset;
} cpu_caps_t;

/**
 * @brief Global cached CPU capability structure
 */
extern cpu_caps_t cpu_caps;

/**
 * @brief Populate the global CPU capability structure
 *
 * Performs a one-time CPUID probe at boot and caches only the subset of
 * CPU capabilities that Retro Rocket actually cares about for runtime
 * decisions (primarily post-2012 / post-baseline features).
 *
 * @details
 * The reported feature set is intentionally incomplete. This is not a full
 * architectural capability dump. Baseline x86_64 features (e.g. SSE2, FXSAVE,
 * etc.) are assumed to be present and are not recorded.
 *
 * In virtualised environments (e.g. KVM/QEMU), the CPUID information may be:
 * - filtered (features hidden),
 * - synthesised (features exposed that the physical CPU does not have),
 * - or incomplete (partial leaf exposure).
 *
 * As a result, the cached feature flags represent what the current execution
 * environment allows, not necessarily the full physical CPU capabilities.
 *
 * For example, older CPUs without CPUID leaf 7 may report very few features,
 * while hypervisors may expose a limited or inconsistent subset of newer
 * features such as UMIP without exposing others from the same generation.
 *
 * This behaviour is expected and should not be interpreted as the CPU being
 * "restricted" — only that the optional features tracked by this structure
 * are not available or not exposed.
 */
void cpu_caps_init(void);
