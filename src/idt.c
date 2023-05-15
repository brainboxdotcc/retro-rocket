#include <kernel.h>

volatile idt_ptr_t idt64 = { sizeof(idt_entry_t) * 64, NULL };

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC_EOI		0x20		/* End-of-interrupt command code */
#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

void io_wait()
{
	outb(0x80, 0);
}

void pic_remap(int offset1, int offset2)
{
	uint8_t a1, a2;

	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}

void pic_disable()
{
	/* Disable PIC */
	uint8_t disable = 0xff;
	outb(PIC1_DATA, disable);
	outb(PIC2_DATA, disable);
}

void pic_enable()
{
	/* Disable PIC */
	uint8_t enable = 0;
	outb(PIC1_DATA, enable);
	outb(PIC2_DATA, enable);
}

void pic_eoi(int irq)
{
	if(irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

void init_idt()
{
	/* Allocate memory for IDT */
	uint32_t base_32 = kmalloc_low(sizeof(idt_entry_t) * 64);
	uint64_t base_64 = (uint64_t)base_32;
	idt64.base = (idt_entry_t*)base_64;

	dprintf("Allocated idt64 at %llx\n", base_64);

	init_error_handler();
	dprintf("Register timer handler\n");
	register_interrupt_handler(IRQ0, timer_callback, dev_zero, NULL);
	dprintf("Register debug\n");
	init_debug();

	/* Fill the IDT with vector addresses */
	idt_init((idt_entry_t*)idt64.base);

	dprintf("IDT64 initialised at %llx\n", &idt64);

	/* load IDT pointer */
	__asm__ volatile("lidtq (%0)\n"::"r"(&idt64));

	/* Unmask and configure each interrupt on the IOAPIC
	 * 24 pins: http://web.archive.org/web/20161130153145/http://download.intel.com/design/chipsets/datashts/29056601.pdf
	 */
	/*for (int in = 0; in < 24; in++) {
		// Unmasked, Active low, level triggered, interrupt mapped to irq + 32
		dprintf("IOAPIC redirection set %d -> %d\n", in, in + 32);
		ioapic_redir_set(in, in + 32, 0, 0, 1, 1, 0);
	}*/

	/* PIT timer
	 */
	uint32_t frequency = 50;
	uint32_t microsecs_divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	outb(0x40, (uint8_t)microsecs_divisor & 0xFF);
	outb(0x40, (uint8_t)microsecs_divisor >> 8);

	pic_remap(0x20, 0x28);
	pic_enable();

	/* Now we are safe to enable interrupts */
	interrupts_on();
	dprintf("Interrupts enabled!\n");
}
