/**
 * @file idt.c
 * @brief IDT and low level interrupt setup
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#include "interrupt.h"
#include "printf.h"
#include <kernel.h>
#include <fred.h>

void pic_disable();
void pic_remap(int offset1, int offset2);

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

#define MSI_VECTORS 256
#define FIRST_MSI_VECTOR 64
#define MSI_WORDS (MSI_VECTORS / 64)

#define MSI_RESERVED_MASK ((FIRST_MSI_VECTOR == 64) ? ~0ULL : ((1ULL << FIRST_MSI_VECTOR) - 1ULL))

static uint64_t msi_bitmap[MAX_CPUS][MSI_WORDS] = {
	[0 ... MAX_CPUS-1] = { MSI_RESERVED_MASK, 0, 0, 0 }
};

bool fred_enabled = true;

__attribute__((aligned(16))) idt_entry_t idt_entries[256];

volatile idt_ptr_t idt64 = {
	.limit = sizeof(idt_entries) - 1,
	.base = idt_entries
};

void interrupt_bsp_common_early_init(void)
{
	init_error_handler();
#ifndef USE_IOAPIC
	register_interrupt_handler(IRQ0, timer_callback, dev_zero, NULL);
#endif
}

void interrupt_bsp_program_pit(void)
{
	uint32_t frequency = 50;
	uint32_t divisor = 1193180 / frequency;

	outb(0x43, 0x36);
	outb(0x40, divisor & 0xFF);
	outb(0x40, divisor >> 8);
}

void interrupt_bsp_route_irqs(void)
{
	pic_remap(0x20, 0x28);
#ifdef USE_IOAPIC
	pic_disable();
	cpu_serialise();
	remap_irqs_to_ioapic();
	cpu_serialise();
	init_lapic_timer(1000);
	cpu_serialise();
	apic_setup_ap();
#else
	pic_enable();
#endif
}

void output_interrupt_mechanism(const char* mechanism)
{
	kprintf("Interrupt delivery method: ");
	setforeground(COLOUR_LIGHTYELLOW);
	kprintf("%s\n", mechanism);
	setforeground(COLOUR_WHITE);
}

void interrupt_bsp_common_late_init(void)
{
	acpi_claim_deferred_irqs();
	interrupts_on();
	dprintf("Interrupts enabled!\n");
}

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

	interrupt_bsp_common_early_init();

	idt_init(idt_entries);

	__asm__ volatile("lidtq (%0)" :: "r"(&idt64));

	interrupt_bsp_program_pit();
	interrupt_bsp_route_irqs();
	output_interrupt_mechanism("IDT");
	interrupt_bsp_common_late_init();
}

void init_interrupts() {
	if (fred_enabled && init_fred()) {
		return;
	}

	init_idt();
}

/* Allocate a free vector on a given CPU (lapic_id) */
int alloc_msi_vector(uint8_t cpu) {
	for (uint8_t w = 0; w < MSI_WORDS; w++) {
		uint64_t free = ~msi_bitmap[cpu][w];
		if (free) {
			int bit = __builtin_ctzll(free);
			msi_bitmap[cpu][w] |= (1ULL << bit);
			return w * 64 + bit;
		}
	}
	return -1; /* none free */
}

void free_msi_vector(uint8_t cpu, int vec) {
	if (vec < FIRST_MSI_VECTOR || vec >= MSI_VECTORS)
		return;
	int w   = vec / 64;
	int bit = vec % 64;
	msi_bitmap[cpu][w] &= ~(1ULL << bit);
}

void load_ap_shared_idt() {
	__asm__ volatile("lidtq (%0)" :: "r"(&idt64));
	interrupts_on();
}

void load_ap_shared_interrupts() {
	if (fred_enabled && enable_fred_for_this_cpu()) {
		interrupts_on();
		return;
	}

	load_ap_shared_idt();
}
