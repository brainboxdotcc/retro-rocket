#include "../include/kernel.h"
#include "../include/interrupt.h"

/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(u64 isrnumber, u64 errorcode)
{
	printf("Interrupt %d\n", isrnumber);
	if (isrnumber != 5)
		wait_forever();
}

void IRQ(u64 isrnumber, u64 errorcode)
{
	printf("IRQ %d\n", isrnumber);
}

