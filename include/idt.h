/**
 * @file idt.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

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
 * @note MAX_CPUS is set to 256 (not the expected physical core count).
 *
 * Local APIC IDs in xAPIC mode are 8-bit fields (0–255). These IDs are not
 * guaranteed to be sequential, zero-based, or densely packed. Some systems
 * leave gaps in the LAPIC ID space, and the bootstrap processor (BSP) may
 * not be assigned LAPIC ID 0.
 *
 * In x2APIC mode, APIC IDs extend to 32 bits. If firmware enables x2APIC
 * before handoff, we accept it and operate in that mode, but we currently
 * only initialise CPUs whose LAPIC IDs are <= 254. LAPIC ID 255 is reserved
 * for broadcast and never used. Any CPUs with IDs above 254 are ignored.
 *
 * By sizing arrays to 256, we can index directly by LAPIC ID without needing
 * a LAPIC→OS CPU remapping table. This simplifies per‑CPU structures at the
 * cost of a small amount of extra memory. Future support for >254 LAPIC IDs
 * may require remapping or dynamic allocation.
 */
#define MAX_CPUS 256


/**
 * @brief Function pointer type for interrupt and IRQ handlers.
 *
 * This defines the signature of an interrupt service routine (ISR) that may be
 * registered via `register_interrupt_handler`. It will be called whenever a
 * matching interrupt or IRQ occurs.
 *
 * All handlers receive the interrupt vector number, error code (if any),
 * the IRQ number (for hardware IRQs, this is `isrnumber - 32`), and a user-defined
 * opaque pointer that was passed during registration. This allows for stateful
 * or device-specific behaviour in shared or reentrant interrupt contexts.
 *
 * @param isrnumber The IDT entry number (0–255), corresponding to the triggered interrupt vector.
 * @param errorcode A CPU-generated error code for certain exceptions (e.g. page faults). Always 0 for IRQs.
 * @param irqnumber For IRQs, this is typically `isrnumber - 32`; otherwise 0.
 * @param opaque The same pointer that was passed to `register_interrupt_handler`; used for context or state.
 */
typedef void (*isr_t)(uint8_t isrnumber, uint64_t errorcode, uint64_t irqnumber, void* opaque);

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
 * @brief Allocate a free MSI interrupt vector on a given CPU.
 *
 * Allocates an IDT vector in the range 64–255 for use with
 * Message Signalled Interrupts (MSI/MSI-X), associated with
 * the specified Local APIC ID.
 *
 * @param cpu Logical CPU ID of the target CPU.
 *
 * @return Vector number (64–255) on success,
 *         -1 if no free vector is available on that CPU.
 *
 * @note After allocation, the driver must program the device’s
 *       MSI/MSI-X capability structure with the returned vector
 *       and Local APIC ID, and register an interrupt handler
 *       for the vector on the specified CPU.
 */
int alloc_msi_vector(uint8_t cpu);

/**
 * @brief Free a previously allocated MSI interrupt vector on a given CPU.
 *
 * Marks the given MSI vector as available for reuse on the specified
 * Local APIC ID.
 *
 * @param cpu Logical CPU ID of the CPU that the vector belongs to.
 * @param vec    The MSI vector to free (64–255).
 *
 * @warning Behaviour is undefined if freeing a vector that
 *          was never allocated on the given CPU, or is still in use.
 *
 * @note Drivers should call this during teardown to avoid
 *       leaking interrupt vectors on that CPU.
 */
void free_msi_vector(uint8_t cpu, int vec);

