#include <kernel.h>
#include <interrupt.h>
#include <hydrogen.h>

/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(u64 isrnumber, u64 errorcode)
{
	// For exceptions, for now we just halt.
	// Most of these are fatal for the moment until we get userland up.
	if (isrnumber < 32)
	{
		printf("CPU halted.\n");
		wait_forever();
	}
}

void IRQ(u64 isrnumber, u64 irqnum)
{
	*((volatile u32*)(hydrogen_info->lapic_paddr + 0xB0)) = 0;
}

