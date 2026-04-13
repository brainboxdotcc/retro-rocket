/**
 * @file fred.c                                                   ~~~~
 * @author Craig Edwards (craigedwards@brainbox.cc)              []
 * @copyright Copyright (c) 2012-2026                            []
 * FRED: Flexible Return and Event Delivery                      []
 *                                                               []
 * Like Fred Dibnah at the foot of a mill chimney, this only    [  ]
 * works if every stack is there, upright, and built before     [  ]
 * lighting it!                                                [_n__]
 * -- https://en.wikipedia.org/wiki/Fred_Dibnah
 *
 * Implemented with lots of assistance from Evalyn Goemer!
 * References:
 * https://evalyngoemer.com/blog/2026/04/11/implementing-fred/
 * https://git.evalyngoemer.com/evalynOS/evalynOS/src/branch/main/kernel/src/arch/x86_64/drivers/fred/fred.c
 * https://git.evalyngoemer.com/evalynOS/evalynOS/src/branch/main/kernel/src/arch/x86_64/drivers/fred/fred.h
 * https://git.evalyngoemer.com/evalynOS/evalynOS/src/branch/main/kernel/src/arch/x86_64/drivers/fred/fred.asm
 */
#include <kernel.h>
#include <fred.h>

extern char fred_entry_page[];

extern void fred_ring3_entry_asm_stub();

bool fred_supported(void)
{
	uint32_t eax = 7, ebx = 0, ecx = 1, edx = 0;
	__asm__ volatile (
		"cpuid"
		: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(eax), "c"(ecx)
		);
	return (eax >> CPUID_FEAT_FRED) & 1u;
}

/**
 * Aligned FRED stacks - we shouldn't need these as we don't switch context
 * from CPL0, but it was being weird without them so better to define them
 * and set them than not.
 */
__attribute__((aligned(64))) static uint8_t fred_stack0[KSTACK_SIZE];
__attribute__((aligned(64))) static uint8_t fred_stack1[USTACK_SIZE];
__attribute__((aligned(64))) static uint8_t fred_stack2[USTACK_SIZE];
__attribute__((aligned(64))) static uint8_t fred_stack3[USTACK_SIZE];

bool enable_fred_for_this_cpu(void)
{
	if (!fred_supported()) {
		return false;
	}

	__asm__ volatile (
		"mov %%cr4, %%rax\n"
		"bts $32, %%rax\n" // set FRED
		"mov %%rax, %%cr4"
		:
		:
		: "rax", "memory"
	);

	wrmsr(IA32_FRED_CONFIG, (uint64_t)fred_ring3_entry_asm_stub);
	wrmsr(IA32_FRED_STKLVLS, 0);

	uint64_t star = ((uint64_t)(0x28) << 48) | ((uint64_t)0x28 << 32);
	wrmsr(IA32_FRED_STAR, star);
	wrmsr(IA32_FRED_RSP0, (uint64_t)(fred_stack0 + KSTACK_SIZE));
	wrmsr(IA32_FRED_RSP1, (uint64_t)(fred_stack1 + USTACK_SIZE));
	wrmsr(IA32_FRED_RSP2, (uint64_t)(fred_stack2 + USTACK_SIZE));
	wrmsr(IA32_FRED_RSP3, (uint64_t)(fred_stack3 + USTACK_SIZE));

	return true;
}

bool init_fred(void)
{
	interrupt_bsp_common_early_init();
	interrupt_bsp_program_pit();
	interrupt_bsp_route_irqs();

	if (!enable_fred_for_this_cpu()) {
		return false;
	}

	output_interrupt_mechanism("FRED");
	interrupt_bsp_common_late_init();

	return true;
}

void fred_ring0_entry(fred_frame_t* frame) {
	uint64_t vector = (frame->ss >> 32) & 0xFF;
	if (vector <= 0x1F) {
		Interrupt(vector, frame->error, frame->ip);
	} else {
		IRQ(vector, vector - 0x20);
	}
}
