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

static inline bool cpuid_has_bit(uint32_t reg, int bit)
{
	return (reg & (1 << bit)) != 0;
}

void cpu_caps_init(void)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	memset(&cpu_caps, 0, sizeof(cpu_caps));

	cpuid_leaf(0, &eax, &ebx, &ecx, &edx);
	cpu_caps.max_basic_leaf = eax;

	memcpy(&cpu_caps.vendor[0], &ebx, 4);
	memcpy(&cpu_caps.vendor[4], &edx, 4);
	memcpy(&cpu_caps.vendor[8], &ecx, 4);
	cpu_caps.vendor[12] = '\0';

	cpuid_leaf(0x80000000, &eax, &ebx, &ecx, &edx);
	cpu_caps.max_extended_leaf = eax;

	if (cpu_caps.max_extended_leaf >= 0x80000004) {
		uint32_t *brand = (uint32_t *)cpu_caps.brand;
		char *start;

		cpuid_leaf(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
		cpuid_leaf(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
		cpuid_leaf(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);

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

	if (cpu_caps.max_basic_leaf >= 7) {
		cpuid_leaf_count(7, 0, &eax, &ebx, &ecx, &edx);

		cpu_caps.fsgsbase = cpuid_has_bit(ebx, 0);
		cpu_caps.smep = cpuid_has_bit(ebx, 7);
		cpu_caps.erms = cpuid_has_bit(ebx, 9);
		cpu_caps.invpcid = cpuid_has_bit(ebx, 10);
		cpu_caps.rdseed = cpuid_has_bit(ebx, 18);
		cpu_caps.adx = cpuid_has_bit(ebx, 19);
		cpu_caps.smap = cpuid_has_bit(ebx, 20);
		cpu_caps.clflushopt = cpuid_has_bit(ebx, 23);
		cpu_caps.clwb = cpuid_has_bit(ebx, 24);
		cpu_caps.sha = cpuid_has_bit(ebx, 29);

		cpu_caps.umip = cpuid_has_bit(ecx, 2);
		cpu_caps.pku = cpuid_has_bit(ecx, 3);
		cpu_caps.ospke = cpuid_has_bit(ecx, 4);
		cpu_caps.waitpkg = cpuid_has_bit(ecx, 5);
		cpu_caps.cet_ss = cpuid_has_bit(ecx, 7);
		cpu_caps.la57 = cpuid_has_bit(ecx, 16);
		cpu_caps.rdpid = cpuid_has_bit(ecx, 22);
		cpu_caps.movdiri = cpuid_has_bit(ecx, 27);
		cpu_caps.movdir64b = cpuid_has_bit(ecx, 28);

		cpu_caps.serialize = cpuid_has_bit(edx, 14);
		cpu_caps.ibt = cpuid_has_bit(edx, 20);

		if (eax >= 1) {
			cpuid_leaf_count(7, 1, &eax, &ebx, &ecx, &edx);

			cpu_caps.fred = cpuid_has_bit(eax, 17);
			cpu_caps.lkgs = cpuid_has_bit(eax, 18);
			cpu_caps.hreset = cpuid_has_bit(eax, 22);
		}
	}
}