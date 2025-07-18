// idt.c - fixed and safe version for up to 256 entries

#include <kernel.h>

#define PIC1         0x20
#define PIC2         0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2+1)
#define PIC_EOI      0x20
#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

// Full IDT with 256 entries
__attribute__((aligned(16)))
idt_entry_t idt_entries[256];

volatile idt_ptr_t idt64 = {
	.limit = sizeof(idt_entries) - 1,
	.base = idt_entries
};

void io_wait() {
	outb(0x80, 0);
}

void pic_remap(int offset1, int offset2) {
	uint8_t a1 = inb(PIC1_DATA);
	uint8_t a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);
	io_wait();
	outb(PIC2_DATA, offset2);
	io_wait();
	outb(PIC1_DATA, 4);
	io_wait();
	outb(PIC2_DATA, 2);
	io_wait();
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);
}

void pic_disable() {
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);
}

void pic_enable() {
	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

void pic_eoi(int irq) {
	if (irq >= 8) {
	outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

void init_idt() {
	memset(idt_entries, 0, sizeof(idt_entries));

	init_error_handler();
	register_interrupt_handler(IRQ0, timer_callback, dev_zero, NULL);
	init_debug();

	// Fill the IDT with handler pointers (in loader.S)
	idt_init(idt_entries);

	__asm__ volatile("lidtq (%0)" :: "r"(&idt64));

	uint32_t frequency = 50;
	uint32_t divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	outb(0x40, divisor & 0xFF);
	outb(0x40, divisor >> 8);

	pic_remap(0x20, 0x28);
	pic_enable();

	interrupts_on();

	dprintf("Interrupts enabled!\n");
}
