#include <kernel.h>
#include <interrupt.h>
#include <hydrogen.h>

isr_t interrupt_handlers[256];

void init_interrupts()
{
	u16 i = 0;
	for (; i < 256; i++)
		interrupt_handlers[i] = NULL;
}

void register_interrupt_handler(u8 n, isr_t handler)
{
	if (interrupt_handlers[n] != 0)
	{
		kprintf("*** BUG *** INT %d claimed twice!\n", n);
		return;
	}
	interrupt_handlers[n] = handler;
}


/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(u64 isrnumber, u64 errorcode)
{
	// For exceptions, for now we just halt.
	// Most of these are fatal for the moment until we get userland up.

	if (interrupt_handlers[isrnumber] != NULL)
	{
		isr_t handler = interrupt_handlers[isrnumber];
		handler((u8)isrnumber, errorcode, 0);
	}

	if (isrnumber < 32)
	{
		printf("CPU halted.\n");
		wait_forever();
	}
}

void IRQ(u64 isrnumber, u64 irqnum)
{
	if (interrupt_handlers[isrnumber] != NULL)
	{
		isr_t handler = interrupt_handlers[isrnumber];
		handler((u8)isrnumber, 0, irqnum);
	}

	*((volatile u32*)(hydrogen_info->lapic_paddr + 0xB0)) = 0;
}

