/**
 * @file io.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Inline helpers for performing privileged I/O port access and interrupt control.
 * @copyright Copyright (c) 2012-2025
 *
 * These functions provide low‑level access to the x86 I/O port space
 * and interrupt enable/disable instructions. They are privileged operations
 * and must only be invoked from kernel mode.
 */
#pragma once

/**
 * @brief Output a single byte to an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @param value Byte to write.
 */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Input a single byte from an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @return Byte read from the port.
 */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
	return value;
}

/**
 * @brief Input a 16‑bit word from an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @return Word read from the port.
 */
static inline uint16_t inw(uint16_t port)
{
	uint16_t value;
	__asm__ volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port));
	return value;
}

/**
 * @brief Output a 16‑bit word to an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @param value Word to write.
 */
static inline void outw(uint16_t port, unsigned short value)
{
	__asm__ volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Input a 32‑bit doubleword from an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @return Doubleword read from the port.
 */
static inline uint32_t inl(uint16_t port)
{
	uint32_t value;
	__asm__ volatile("inl %%dx, %%eax" : "=a" (value) : "d" (port));
	return value;
}

/**
 * @brief Output a 32‑bit doubleword to an I/O port.
 *
 * @param port 16‑bit I/O port address.
 * @param value Doubleword to write.
 */
static inline void outl(uint16_t port, unsigned long value)
{
	__asm__ volatile("outl %%eax, %%dx" : : "d" (port), "a" (value));
}

/**
 * @brief Input a sequence of 32‑bit values from an I/O port.
 *
 * @param port   16‑bit I/O port address.
 * @param buffer Pointer to destination buffer.
 * @param count  Number of doublewords to read.
 */
static inline void insl(uint16_t port, void* buffer, uint32_t count)
{
	uint32_t* b = (uint32_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		b[v] = inl(port);
}

/**
 * @brief Input a sequence of 16‑bit values from an I/O port.
 *
 * @param port   16‑bit I/O port address.
 * @param buffer Pointer to destination buffer.
 * @param count  Number of words to read.
 */
static inline void insw(uint16_t port, void* buffer, uint32_t count)
{
	uint16_t* b = (uint16_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		b[v] = inw(port);
}

/**
 * @brief Output a sequence of 16‑bit values to an I/O port.
 *
 * @param port   16‑bit I/O port address.
 * @param buffer Pointer to source buffer.
 * @param count  Number of words to write.
 */
static inline void outsw(uint16_t port, void* buffer, uint32_t count)
{
	uint16_t* b = (uint16_t*)buffer;
	for (uint32_t v = 0; v < count; v++)
		outw(port, b[v]);
}

/**
 * @brief Enable maskable hardware interrupts.
 *
 * Executes the `sti` instruction.
 */
static inline void interrupts_on()
{
	__asm__ volatile("sti");
}

/**
 * @brief Disable maskable hardware interrupts.
 *
 * Executes the `cli` instruction.
 */
static inline void interrupts_off()
{
	__asm__ volatile("cli");
}

/**
 * @brief Halt the CPU and remain halted forever.
 *
 * Loops indefinitely, executing the `hlt` instruction on each iteration.
 * Used to stop execution in unrecoverable conditions.
 */
static inline void wait_forever()
{
	for (;;) __asm__ volatile("hlt");
}
