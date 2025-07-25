/**
 * @file idt.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

#define USE_IOAPIC

typedef struct idt_ptr_t {
	uint16_t limit;
	void* base;
} __attribute__((packed)) idt_ptr_t;

typedef struct idt_entry_t {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
   uint32_t offset_3;        // offset bits 32..63
   uint32_t zero;            // reserved
} __attribute__((packed)) idt_entry_t;

extern volatile idt_ptr_t idt64;

void init_idt();

/* Function body defined in asm/loader.S */
void idt_init(void* idt);

void pic_eoi(int irq);

void io_wait();