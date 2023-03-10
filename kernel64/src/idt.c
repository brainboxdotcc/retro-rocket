#include <kernel.h>

//u16 idt64[5] = {0xffff, 0x0000, 0x0000, 0x0000, 0x0000 };

/* 64-bit IDT is at &idt */
static unsigned char idt[0x10000];
idt_ptr_t idt64 = { 0xffff, &idt };

void idt_setup()
{
	init_interrupts();
	idt_init(&idt);
	asm volatile("lidtq (%0)\n"::"r"(&idt64));
}

