#include <kernel.h>

//uint16_t idt64[5] = {0xffff, 0x0000, 0x0002, 0x0000, 0x0000 };

/* 64-bit IDT is at &idt */
//idt_entry_t idt[255] = { 0 };
idt_ptr_t idt64 = { sizeof(idt_entry_t) * 255, (idt_entry_t*)0x10000 };

void idt_setup()
{
	init_interrupts();
	idt_init((idt_entry_t*)0x10000);
	asm volatile("lidtq (%0)\n"::"r"(&idt64));
}
