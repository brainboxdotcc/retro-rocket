#ifndef __IO_H__
#define __IO_H__

#include "kprintf.h"
#include "kernel.h"

/* Output one byte to an I/O port. Privileged operation. */
static inline void outb(int port, unsigned char value)
{
	asm volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

/* Read a byte from an I/O port. Privileged operation. */
static inline unsigned char inb(int port)
{
	unsigned char value;
	asm volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline unsigned short inw(int port)
{                                                  
	unsigned short value;
	asm volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline void outw(int port, unsigned short value)
{
	asm volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static inline unsigned long inl(int port)
{
	unsigned long value;
	asm volatile("inl %%dx, %%eax" : "=a" (value) : "d" (port));
	return value;
}

static inline void outl(int port, unsigned long value)
{
	asm volatile("outl %%eax, %%dx" : : "d" (port), "a" (value));
}

#define insl(port, buffer, count) \
	         asm volatile("cld; rep; insl" :: "D" (buffer), "d" (port), "c" (count))

#define insw(port, buffer, count) \
		asm volatile("cld; rep; insw" :: "D" (buffer), "d" (port), "c" (count))

static inline void interrupts_on()
{
	asm volatile("sti");
}

static inline void interrupts_off()
{
	asm volatile("cli");
}

static inline void wait_forever()
{
	for (;;) asm volatile("hlt");
}

#endif
