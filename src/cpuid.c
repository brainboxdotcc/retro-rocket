#include <kernel.h>
#include <rr-cpuid.h>

cpu_caps_t cpu_caps;

static inline void cpuid_leaf(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile (
		"cpuid"
		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(leaf), "c"(0)
		);
}

static inline void cpuid_leaf_count(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile (
		"cpuid"
		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(leaf), "c"(subleaf)
		);
}

static inline bool cpuid_has_bit(uint32_t reg, uint32_t mask)
{
	return (reg & mask) != 0;
}

void cpu_caps_init(void)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	memset(&cpu_caps, 0, sizeof(cpu_caps));

	cpuid_leaf(CPUID_GETVENDORSTRING, &eax, &ebx, &ecx, &edx);
	cpu_caps.max_basic_leaf = eax;

	memcpy(&cpu_caps.vendor[0], &ebx, 4);
	memcpy(&cpu_caps.vendor[4], &edx, 4);
	memcpy(&cpu_caps.vendor[8], &ecx, 4);
	cpu_caps.vendor[12] = '\0';

	cpuid_leaf(CPUID_INTELEXTENDED, &eax, &ebx, &ecx, &edx);
	cpu_caps.max_extended_leaf = eax;

	if (cpu_caps.max_extended_leaf >= CPUID_INTELBRANDSTRINGEND) {
		uint32_t *brand = (uint32_t *)cpu_caps.brand;
		char *start;

		cpuid_leaf(CPUID_INTELBRANDSTRING, &brand[0], &brand[1], &brand[2], &brand[3]);
		cpuid_leaf(CPUID_INTELBRANDSTRINGMORE, &brand[4], &brand[5], &brand[6], &brand[7]);
		cpuid_leaf(CPUID_INTELBRANDSTRINGEND, &brand[8], &brand[9], &brand[10], &brand[11]);

		cpu_caps.brand[48] = '\0';

		start = cpu_caps.brand;
		while (*start == ' ') {
			start++;
		}

		if (start != cpu_caps.brand) {
			memmove(cpu_caps.brand, start, strlen(start) + 1);
		}
	} else {
		strlcpy(cpu_caps.brand, "Unknown CPU", 49);
	}

	cpuid_leaf(CPUID_GETFEATURES, &eax, &ebx, &ecx, &edx);

	cpu_caps.hypervisor_present = (ecx & CPUID_FEAT_ECX_HYPERVISOR) != 0;
	if (cpu_caps.hypervisor_present) {
		cpuid_leaf(0x40000000, &eax, &ebx, &ecx, &edx);

		memcpy(&cpu_caps.hypervisor_vendor[0], &ebx, 4);
		memcpy(&cpu_caps.hypervisor_vendor[4], &edx, 4);
		memcpy(&cpu_caps.hypervisor_vendor[8], &ecx, 4);
		cpu_caps.hypervisor_vendor[12] = '\0';
	}

	if (cpu_caps.max_basic_leaf >= 7) {
		cpuid_leaf_count(7, 0, &eax, &ebx, &ecx, &edx);

		cpu_caps.fsgsbase = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_FSGSBASE);
		cpu_caps.smep = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_SMEP);
		cpu_caps.erms = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_ERMS);
		cpu_caps.invpcid = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_INVPCID);
		cpu_caps.rdseed = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_RDSEED);
		cpu_caps.adx = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_ADX);
		cpu_caps.smap = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_SMAP);
		cpu_caps.clflushopt = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_CLFLUSHOPT);
		cpu_caps.clwb = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_CLWB);
		cpu_caps.sha = cpuid_has_bit(ebx, CPUID_FEAT_7_EBX_SHA);

		cpu_caps.umip = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_UMIP);
		cpu_caps.pku = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_PKU);
		cpu_caps.ospke = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_OSPKE);
		cpu_caps.waitpkg = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_WAITPKG);
		cpu_caps.cet_ss = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_CET_SS);
		cpu_caps.la57 = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_LA57);
		cpu_caps.rdpid = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_RDPID);
		cpu_caps.movdiri = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_MOVDIRI);
		cpu_caps.movdir64b = cpuid_has_bit(ecx, CPUID_FEAT_7_ECX_MOVDIR64B);

		cpu_caps.serialize = cpuid_has_bit(edx, CPUID_FEAT_7_EDX_SERIALIZE);
		cpu_caps.ibt = cpuid_has_bit(edx, CPUID_FEAT_7_EDX_IBT);

		if (eax >= 1) {
			cpuid_leaf_count(7, 1, &eax, &ebx, &ecx, &edx);

			cpu_caps.fred = cpuid_has_bit(eax, CPUID_FEAT_7_1_EAX_FRED);
			cpu_caps.lkgs = cpuid_has_bit(eax, CPUID_FEAT_7_1_EAX_LKGS);
			cpu_caps.hreset = cpuid_has_bit(eax, CPUID_FEAT_7_1_EAX_HRESET);
		}
	}

	dprintf("cpu: %s (%s)\n", cpu_caps.brand, cpu_caps.vendor);
	dprintf("cpu: max basic leaf: %u\n", cpu_caps.max_basic_leaf);

	if (cpu_caps.hypervisor_present) {
		dprintf("cpu: running under hypervisor: %s\n", cpu_caps.hypervisor_vendor);
	} else {
		dprintf("cpu: running on bare metal\n");
	}

	dprintf("cpu: features:\n");

	if (cpu_caps.erms) {
		dprintf("  erms (fast rep movsb/stosb)\n");
	}
	if (cpu_caps.fsgsbase) {
		dprintf("  fsgsbase (fast fs/gs base access)\n");
	}
	if (cpu_caps.smep) {
		dprintf("  smep (no user code execution in kernel)\n");
	}
	if (cpu_caps.smap) {
		dprintf("  smap (no user memory access in kernel)\n");
	}
	if (cpu_caps.umip) {
		dprintf("  umip (restricted user instructions)\n");
	}
	if (cpu_caps.invpcid) {
		dprintf("  invpcid (precise tlb invalidation)\n");
	}
	if (cpu_caps.rdseed) {
		dprintf("  rdseed (hardware entropy)\n");
	}
	if (cpu_caps.adx) {
		dprintf("  adx (multi-precision arithmetic)\n");
	}
	if (cpu_caps.clflushopt) {
		dprintf("  clflushopt (optimised cache flush)\n");
	}
	if (cpu_caps.clwb) {
		dprintf("  clwb (cache line write-back)\n");
	}
	if (cpu_caps.sha) {
		dprintf("  sha (sha instruction extensions)\n");
	}
	if (cpu_caps.pku) {
		dprintf("  pku (memory protection keys)\n");
	}
	if (cpu_caps.ospke) {
		dprintf("  ospke (os-managed protection keys)\n");
	}
	if (cpu_caps.waitpkg) {
		dprintf("  waitpkg (low power wait instructions)\n");
	}
	if (cpu_caps.rdpid) {
		dprintf("  rdpid (fast cpu id read)\n");
	}
	if (cpu_caps.la57) {
		dprintf("  la57 (5-level paging)\n");
	}
	if (cpu_caps.serialize) {
		dprintf("  serialize (execution serialization instruction)\n");
	}
	if (cpu_caps.movdiri) {
		dprintf("  movdiri (direct store instruction)\n");
	}
	if (cpu_caps.movdir64b) {
		dprintf("  movdir64b (64-byte direct store)\n");
	}
	if (cpu_caps.cet_ss) {
		dprintf("  cet_ss (shadow stack)\n");
	}
	if (cpu_caps.ibt) {
		dprintf("  ibt (indirect branch tracking)\n");
	}
	if (cpu_caps.fred) {
		dprintf("  fred (flexible return and event delivery)\n");
	}
	if (cpu_caps.lkgs) {
		dprintf("  lkgs (load kernel gs)\n");
	}
	if (cpu_caps.hreset) {
		dprintf("  hreset (hardware history reset)\n");
	}
}