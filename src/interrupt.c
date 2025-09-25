#include <kernel.h>
#include <interrupt.h>

/**
 * @brief Array of linked lists of interrupt handlers by ISR number.
 * Each ISR may have zero or more handlers attached. a NULL here means an empty list,
 * and no handlers.
 */
static shared_interrupt_t* shared_interrupt[256];

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
		dprintf("NOTE: %s %d is shared!\n", n < IRQ_START ? "ISR" : "IRQ", n < IRQ_START ? n : n - IRQ_START);
	}
	if (n >= IRQ_START && !si->next) {
		dprintf("Unmasking irq %u\n", n - IRQ_START);
		ioapic_mask_set(n - IRQ_START, false); // Unmask
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

			/* If this was the last handler, mask the IRQ again */
			if (!shared_interrupt[n] && n >= IRQ_START) {
				ioapic_mask_set(n - IRQ_START, true);
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

void Interrupt(uint64_t isrnumber, uint64_t errorcode, uint64_t rip) {
	__attribute__((aligned(16))) uint8_t fx[512];
	__builtin_ia32_fxsave64(&fx);

	for (shared_interrupt_t *si = shared_interrupt[isrnumber]; si; si = si->next) {
		/* There is no shared interrupt routing on these interrupts,
		 * they are purely routed to interested handlers
		 */
		if (si->interrupt_handler) {
			si->interrupt_handler((uint8_t) isrnumber, errorcode, rip, si->opaque);
		}
	}

	if (isrnumber < 32) {
		/* This simple error handler is replaced by a more complex debugger once the system is up */
		kprintf("CPU %d halted with exception %016lx, error code %016lx: %s.\n", logical_cpu_id(), isrnumber, errorcode, error_table[isrnumber]);
		wait_forever();
	}

	entropy_irq_event();
	local_apic_clear_interrupt();
	__builtin_ia32_fxrstor64(&fx);
}

void IRQ(uint64_t isrnumber, uint64_t irqnum)
{
	__attribute__((aligned(16))) uint8_t fx[512];
	__builtin_ia32_fxsave64(&fx);

	/* On shared INTx lines, always call all registered handlers.
	 * Each device ISR must check/clear its own cause registers.
	 */
	for (shared_interrupt_t* si = shared_interrupt[isrnumber]; si; si = si->next) {
		if (si->interrupt_handler) {
			si->interrupt_handler((uint8_t)isrnumber, 0, irqnum, si->opaque);
		}
	}

	entropy_irq_event();
	local_apic_clear_interrupt();
	__builtin_ia32_fxrstor64(&fx);
}
