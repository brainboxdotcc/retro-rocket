#ifndef __IDT_H__
#define __IDT_H__

#include <kernel.h>

struct idt_ptr {
	u16 limit;
	void* base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

extern idt_ptr_t idt64;

void idt_setup();

/* Function body defined in asm/loader.S */
void idt_init(void* idt);

#endif
