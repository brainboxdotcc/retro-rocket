#ifndef __IO_H__
#define __IO_H__

/* Output one byte to an I/O port. Privileged operation. */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

/* Read a byte from an I/O port. Privileged operation. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline uint16_t inw(uint16_t port)
{                                                  
	uint16_t value;
	__asm__ volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port));
	return value;
}                               

static inline void outw(uint16_t port, unsigned short value)
{
	__asm__ volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t value;
	__asm__ volatile("inl %%dx, %%eax" : "=a" (value) : "d" (port));
	return value;
}

static inline void outl(uint16_t port, unsigned long value)
{
	__asm__ volatile("outl %%eax, %%dx" : : "d" (port), "a" (value));
}

static inline void insl(uint16_t port, void* buffer, uint32_t count)
{
	uint32_t* b = (uint32_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		b[v] = inl(port);
}

static inline void insw(uint16_t port, void* buffer, uint32_t count)
{
	uint16_t* b = (uint16_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		b[v] = inw(port);
}

static inline void outsw(uint16_t port, void* buffer, uint32_t count)
{
	uint16_t* b = (uint16_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		outw(port, b[v]);
}


static inline void interrupts_on()
{
	__asm__ volatile("sti");
}

static inline void interrupts_off()
{
	__asm__ volatile("cli");
}

static inline void wait_forever()
{
	for (;;) __asm__ volatile("hlt");
}

#endif
