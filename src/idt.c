#include <kernel.h>

volatile idt_ptr_t idt64 = { sizeof(idt_entry_t) * 255, NULL };

void init_idt()
{
	/* Allocate memory for IDT */
	uint32_t base_32 = kmalloc_low(sizeof(idt_entry_t) * 255);
	uint64_t base_64 = (uint64_t)base_32;
	idt64.base = (idt_entry_t*)base_64;

	/* Fill the IDT with vector addresses */
	idt_init((idt_entry_t*)idt64.base);

	/* load IDT pointer */
	__asm__ volatile("lidtq (%0)\n"::"r"(&idt64));

	/* Unmask and configure each interrupt on the IOAPIC */
	for (int in = 0; in < 24; in++) {
		ioapic_redir_unmask(in);
		/* Active low, level triggered, interrupt mapped to irq + 32 */
		ioapic_redir_set(in, in + 32, 0, 0, 1, 1, 0);
	}

	/* Now we are safe to enable interrupts */
	interrupts_on();
}
