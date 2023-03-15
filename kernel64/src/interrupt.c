#include <kernel.h>
#include <interrupt.h>

isr_t interrupt_handlers[256] = { 0 };

void init_interrupts()
{
}

void register_interrupt_handler(uint8_t n, isr_t handler)
{
	if (interrupt_handlers[n] != 0 && interrupt_handlers[n] != handler)
	{
		kprintf("*** BUG *** INT %d claimed twice!\n", n);
		return;
	}
	interrupt_handlers[n] = handler;
	//kprintf("Registered handler %d to %016llx\n", n, &handler);
}


/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(uint64_t isrnumber, uint64_t errorcode)
{
	// For exceptions, for now we just halt.
	// Most of these are fatal for the moment until we get userland up.
	kprintf("Interrupt %d\n", isrnumber);

	if (interrupt_handlers[isrnumber] != NULL)
	{
		isr_t handler = interrupt_handlers[isrnumber];
		handler((uint8_t)isrnumber, errorcode, 0);
	}

	if (isrnumber < 32)
	{
		printf("CPU %d halted with exception %016x, error code %016x.\n", cpu_id(), isrnumber, errorcode);
		wait_forever();
	}
	*((volatile uint32_t*)(0xFEE00000 + 0xB0)) = 0;
}

void IRQ(uint64_t isrnumber, uint64_t irqnum)
{
	if (irqnum > 0)
		kprintf("IRQ %d\n", irqnum);
	if (interrupt_handlers[isrnumber] != NULL)
	{
		isr_t handler = interrupt_handlers[isrnumber];
		handler((uint8_t)isrnumber, 0, irqnum);
	}

	if (irqnum != IRQ7) {
		*((volatile uint32_t*)(0xFEE00000 + 0xB0)) = 0;
	}
}

