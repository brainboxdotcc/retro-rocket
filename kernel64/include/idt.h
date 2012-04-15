#ifndef __IDT_H__
#define __IDT_H__

/* Function body defined in asm/idt.S */

extern u16 idt64[5];
void idt_setup();
void idt_init();

#endif
