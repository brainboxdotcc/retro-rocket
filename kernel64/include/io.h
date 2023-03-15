#ifndef __IO_H__
#define __IO_H__

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

#define insl(port, buffer, count) { int v = 0; uint32_t* b = (uint32_t*)buffer; for (; v < count; v++) b[v] = inl(port); }

#define insw(port, buffer, count) { int v = 0; uint16_t* b = (uint16_t*)buffer; for (; v < count; v++) b[v] = inw(port); }

#define outsw(port, buffer, count) { int v = 0; uint16_t* b = (uint16_t*)buffer; for (; v < count; v++) outw(port, b[v]); }


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
