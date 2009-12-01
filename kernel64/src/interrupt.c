#include "../include/kernel.h"
#include "../include/interrupt.h"

/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(u64 isrnumber, u64 errorcode)
{
	printf("Interrupt %d\n", isrnumber);

	// For exceptions, for now we just halt.
	// Most of these are fatal for the moment until we get userland up.
	if (isrnumber < 32)
		wait_forever();
}

void IRQ(u64 isrnumber, u64 errorcode)
{
	printf("IRQ %d\n", isrnumber);
}

