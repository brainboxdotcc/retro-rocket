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

/**
 * @brief Initialise the IDT and enable interrupts.
 *
 * Sets up the full 256-entry Interrupt Descriptor Table (IDT),
 * remaps or disables the legacy PIC depending on configuration,
 * configures the PIT to 50Hz (if APIC timer is not used), and
 * loads the IDT register. CPU interrupts are enabled at the end.
 *
 * @note Called once during kernel bootstrap. Drivers must not
 *       call this directly.
 */
void init_idt(void);

/**
 * @brief Low-level assembly routine to populate the IDT.
 *
 * Fills the IDT with ISR and IRQ stub addresses defined in
 * loader.S.
 *
 * @param idt Pointer to the IDT base.
 *
 * @note Invoked internally by init_idt(). Not for driver use.
 */
void idt_init(void* idt);

/**
 * @brief Send End-of-Interrupt (EOI) to the PIC.
 *
 * Notifies the PIC(s) that interrupt handling has finished.
 * For IRQs >= 8, signals both slave and master PICs.
 *
 * @param irq The IRQ line number (0–15).
 *
 * @note Normally issued automatically by the kernel after
 *       invoking registered handlers. Only call directly when
 *       writing low-level PIC-specific code.
 */
void pic_eoi(int irq);

/**
 * @brief Small I/O delay for hardware synchronisation.
 *
 * Performs an I/O write to port 0x80 to allow hardware devices
 * (such as the PIC) time to settle after reprogramming.
 *
 * @note Typically only needed by low-level kernel code. Most
 *       drivers do not need to call this.
 */
void io_wait(void);

/**
 * @brief Allocate a free MSI interrupt vector.
 *
 * Allocates an IDT vector in the range 64–255 for use with
 * Message Signalled Interrupts (MSI/MSI-X).
 *
 * @return Vector number (64–255) on success,
 *         -1 if no free vector is available.
 *
 * @note After allocation, the driver must program the device’s
 *       MSI capability structure and register an interrupt
 *       handler for the vector.
 */
int alloc_msi_vector(void);

/**
 * @brief Free a previously allocated MSI interrupt vector.
 *
 * Marks the given MSI vector as available for reuse.
 *
 * @param vec The MSI vector to free (64–255).
 *
 * @warning Behaviour is undefined if freeing a vector that
 *          was never allocated or is still in use.
 *
 * @note Drivers should call this during teardown to avoid
 *       leaking interrupt vectors.
 */
void free_msi_vector(int vec);
