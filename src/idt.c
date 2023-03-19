#include <kernel.h>

//uint16_t idt64[5] = {0xffff, 0x0000, 0x0002, 0x0000, 0x0000 };

/* 64-bit IDT is at &idt */
//idt_entry_t idt[255] = { 0 };
idt_ptr_t idt64 = { sizeof(idt_entry_t) * 255, NULL };

void idt_setup()
{
	uint32_t base_32 = kmalloc_low(sizeof(idt_entry_t) * 255);
	uint64_t base_64 = (uint64_t)base_32;
	idt64.base = (idt_entry_t*)base_64;
	init_interrupts();
	idt_init((idt_entry_t*)idt64.base);
	asm volatile("lidtq (%0)\n"::"r"(&idt64));
}
