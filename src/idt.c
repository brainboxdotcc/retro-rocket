#include <kernel.h>

volatile idt_ptr_t idt64 = { sizeof(idt_entry_t) * 255, NULL };

void idt_setup()
{
	/* Allocate memory for IDT */
	uint32_t base_32 = kmalloc_low(sizeof(idt_entry_t) * 255);
	uint64_t base_64 = (uint64_t)base_32;
	idt64.base = (idt_entry_t*)base_64;

	/* Fill the IDT with vector addresses */
	idt_init((idt_entry_t*)idt64.base);

	/* load IDT pointer */
	asm volatile("lidtq (%0)\n"::"r"(&idt64));

	/* Unmask and configure each interrupt on the IOAPIC */
	for (int in = 0; in < 16; in++) {
		ioapic_redir_unmask(in);
		ioapic_redir_set_precalculated(in, 0, 0x2020 + in);
	}

	/* Now we are safe to enable interrupts */
	interrupts_on();
}
