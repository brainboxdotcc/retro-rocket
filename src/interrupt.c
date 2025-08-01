#include <kernel.h>
#include <interrupt.h>

/**
 * @brief Array of linked lists of interrupt handlers by ISR number.
 * Each ISR may have zero or more handlers attached. a NULL here means an empty list,
 * and no handlers.
 */
shared_interrupt_t* shared_interrupt[256] = { 0 };

#define IRQ_VECTOR_BASE 0x20 // IDT[32]–IDT[47] for IRQ0–IRQ15

extern const char* const error_table[];

void remap_irqs_to_ioapic() {
	for (uint8_t irq = 0; irq < 24; ++irq) {
		uint32_t vector = IRQ_VECTOR_BASE + irq;
		ioapic_redir_set(
			irq,
			vector,
			0,  // del_mode 0: fixed
			0,  // dest_mode 0: physical
			get_irq_polarity(irq),  // intpol (0 = active high)
			get_irq_trigger_mode(irq),  // trigger_mode (0 = edge)
			1 // masked
		);
	}
}

bool register_interrupt_handler(uint8_t n, isr_t handler, pci_dev_t device, void* opaque) {
	shared_interrupt_t* si = kmalloc(sizeof(shared_interrupt_t));
	if (!si) {
		return false;
	}
	si->device = device;
	si->interrupt_handler = handler;
	si->next = shared_interrupt[n];
	si->opaque = opaque;
	shared_interrupt[n] = si;
	if (si->next) {
		dprintf("NOTE: %s %d is shared!\n", n < 32 ? "ISR" : "IRQ", n < 32 ? n : n - 32);
	}
	if (n >= 32) {
		ioapic_mask_set(n - 32, false); // Unmask
	}
	return true;
}

bool deregister_interrupt_handler(uint8_t n, isr_t handler) {
	shared_interrupt_t* current = shared_interrupt[n];
	shared_interrupt_t* prev = NULL;

	while (current) {
		if (current->interrupt_handler == handler) {
			if (prev) {
				prev->next = current->next;
			} else {
				shared_interrupt[n] = current->next;
			}
			kfree_null(&current);

			// If this was the last handler, mask the IRQ again
			if (!shared_interrupt[n] && n >= 32) {
				ioapic_mask_set(n - 32, true);
			}
			return true;
		}
		prev = current;
		current = current->next;
	}
	return false; // Handler not found
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
		 * they are purely routed to interested handlers
		 */
		if (si->interrupt_handler) {
			si->interrupt_handler((uint8_t)isrnumber, errorcode, 0, si->opaque);
		}
	}

	if (isrnumber < 32) {
		/* This simple error handler is replaced by a more complex debugger once the system is up */
		kprintf("CPU %d halted with exception %016lx, error code %016lx: %s.\n", cpu_id(), isrnumber, errorcode, error_table[isrnumber]);
		wait_forever();
	}

#ifdef USE_IOAPIC
	local_apic_clear_interrupt();
#else
	pic_eoi(isrnumber);
#endif
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
#ifdef USE_IOAPIC
	local_apic_clear_interrupt();
#else
	/* IRQ7 is the PIC spurious interrupt, we never acknowledge it */
	if (irqnum != IRQ7) {
		pic_eoi(isrnumber);
	}
#endif
	__builtin_ia32_fxrstor64(&fx);
}

