#include <kernel.h>
#include <interrupt.h>

/**
 * @brief Array of linked lists of interrupt handlers by ISR number.
 * Each ISR may have zero or more handlers attached. a NULL here means an empty list,
 * and no handlers.
 */
shared_interrupt_t* shared_interrupt[256] = { 0 };

void register_interrupt_handler(uint8_t n, isr_t handler, pci_dev_t device, void* opaque)
{
	shared_interrupt_t* si = kmalloc(sizeof(shared_interrupt_t));
	si->device = device;
	si->interrupt_handler = handler;
	si->next = shared_interrupt[n];
	si->opaque = opaque;
	shared_interrupt[n] = si;
	if (si->next) {
		dprintf("NOTE: %s %d is shared!\n", n < 32 ? "ISR" : "IRQ", n < 32 ? n : n - 32);
	}
}

/**
 * @brief Clear local APIC interrupt
 */
void local_apic_clear_interrupt()
{
	apic_write(APIC_EOI, 0);
}

/* Both the Interrupt() and ISR() functions are dispatched from the assembly code trampoline via a pre-set IDT */

void Interrupt(uint64_t isrnumber, uint64_t errorcode)
{
	uint8_t fx[512];
	__builtin_ia32_fxsave64(&fx);
	for (shared_interrupt_t* si = shared_interrupt[isrnumber]; si; si = si->next) {
		/* There is no shared interrupt routing on these interrupts,
		 * they are purely routed to interested non-pci handlers
		 */
		if (si->device.bits == 0 && si->interrupt_handler) {
			si->interrupt_handler((uint8_t)isrnumber, errorcode, 0, si->opaque);
		}
	}

	if (isrnumber < 32) {
		/* This simple error handler is replaced by a more complex debugger once the system is up */
		dprintf("CPU %d halted with exception %016lx, error code %016lx.\n", cpu_id(), isrnumber, errorcode);
		wait_forever();
	}

	//local_apic_clear_interrupt();
	pic_eoi(isrnumber);
	__builtin_ia32_fxrstor64(&fx);
}

void IRQ(uint64_t isrnumber, uint64_t irqnum)
{
	uint8_t fx[512];
	__builtin_ia32_fxsave64(&fx);
	for (shared_interrupt_t* si = shared_interrupt[isrnumber]; si; si = si->next) {
		if (si->device.bits == 0 && si->interrupt_handler) {
			/* Not a PCI device, we just dispatch to the handler */
			si->interrupt_handler((uint8_t)isrnumber, 0, irqnum, si->opaque);
		} else {
			/* PCI device, check if this device is signalled */
			uint16_t status = pci_read(si->device, PCI_STATUS);
			if (status & PCI_STATUS_INTERRUPT && si->interrupt_handler) {
				/* This device is signalled, dispatch to its handler */
				si->interrupt_handler((uint8_t)isrnumber, 0, irqnum, si->opaque);
			}
		}
	}
	/* IRQ7 is the APIC spurious interrupt, we never acknowledge it */
	if (irqnum != IRQ7) {
		//local_apic_clear_interrupt();
		pic_eoi(isrnumber);
	}
	__builtin_ia32_fxrstor64(&fx);
}

