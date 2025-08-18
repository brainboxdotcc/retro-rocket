/**
 * @file interrupt.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

/**
 * @brief Mappings of hardware IRQ lines to remapped IDT interrupt vectors.
 *
 * IRQs are remapped to avoid conflicts with CPU exceptions (0–31).
 * This enum provides named constants for common hardware IRQs,
 * starting at interrupt vector 32.
 */
enum irq_number_t {
	IRQ_START = 32,  ///< Base offset for remapped IRQs

	IRQ0  = 32,  ///< System timer (PIT)
	IRQ1  = 33,  ///< Keyboard controller
	IRQ2  = 34,  ///< Cascade for IRQs 8–15 (used internally by PIC)
	IRQ3  = 35,  ///< Serial port COM2
	IRQ4  = 36,  ///< Serial port COM1
	IRQ5  = 37,  ///< Sound card or other devices
	IRQ6  = 38,  ///< Floppy disk controller
	IRQ7  = 39,  ///< Parallel port or spurious IRQ

	IRQ8  = 40,  ///< Real-time clock (RTC)
	IRQ9  = 41,  ///< ACPI, network, or SCSI
	IRQ10 = 42,  ///< Available for devices (network, SCSI, etc.)
	IRQ11 = 43,  ///< Available for devices (network, SCSI, etc.)
	IRQ12 = 44,  ///< PS/2 mouse
	IRQ13 = 45,  ///< x87 Floating Point Unit
	IRQ14 = 46,  ///< Primary ATA/IDE channel
	IRQ15 = 47,  ///< Secondary ATA/IDE channel

	IRQ16 = 50,  ///< Local APIC timer vector (used instead of IRQ0 in APIC mode)
};

/**
 * @brief Represents a single handler in a chain of handlers attached to a shared ISR.
 *
 * Some IRQ lines may be shared between multiple devices (especially in legacy or
 * MSI-less systems). This structure forms a linked list of handlers associated
 * with a given interrupt vector. Each node contains the handler function, the
 * originating PCI device (used to support IRQ sharing), and an opaque pointer
 * passed to the handler at runtime.
 */
typedef struct shared_interrupt_t {
	isr_t interrupt_handler;         ///< Function pointer to the registered interrupt handler
	pci_dev_t device;                ///< Associated PCI device for shared IRQ identification
	void* opaque;                    ///< User-defined pointer passed to the handler
	struct shared_interrupt_t* next; ///< Next handler in the linked list (if multiple handlers share this ISR)
} shared_interrupt_t;


/**
 * @brief Register an interrupt handler for a given ISR or IRQ line.
 *
 * This function attaches a handler to an interrupt vector `n`, allowing
 * multiple devices to share the same IRQ line via a linked list of handlers.
 * The handler will be invoked when the interrupt occurs, with the given
 * `opaque` pointer passed back as context.
 *
 * If the interrupt number `n` is 32 or higher, it is assumed to be an IRQ
 * (as per the `irq_number_t` scheme), and the corresponding I/O APIC input
 * line is unmasked automatically. This enables delivery of the IRQ to the
 * local APIC once registered.
 *
 * For IRQ sharing, you must provide the `device` field correctly so that
 * handlers for multiple PCI devices on the same line can be tracked and
 * dispatched safely. This is a requirement of shared interrupt delivery.
 *
 * @param n Interrupt number - 0–31 are reserved ISRs; 32+ are IRQs.
 * @param handler Function pointer to the interrupt handler.
 * @param device For PCI devices, specify the device node responsible.
 *               Used to manage IRQ sharing.
 * @param opaque A user-defined pointer passed to the handler on invocation.
 * @return `false` if memory allocation failed or registration was unsuccessful;
 *         `true` otherwise.
 */
bool register_interrupt_handler(uint8_t n, isr_t handler, pci_dev_t device, void* opaque);


/**
 * @brief Global entry point for CPU exceptions and software ISRs (ISR < 32).
 *
 * This function is the common handler for processor-generated exceptions
 * (such as divide-by-zero, page faults, general protection faults, etc.)
 * and any custom software ISRs within the reserved range 0–31.
 *
 * It is invoked directly from the IDT when an exception occurs. The
 * `errorcode` parameter is provided by the CPU and may contain relevant
 * fault information depending on the exception type.
 *
 * @param isrnumber The interrupt service routine number (0–31).
 * @param errorcode The associated CPU-generated error code (or zero if unused).
 */
void Interrupt(uint64_t isrnumber, uint64_t errorcode);

/**
 * @brief Global entry point for hardware IRQs (ISR ≥ 32, remapped).
 *
 * This function is invoked by the interrupt dispatcher when a hardware
 * interrupt request (IRQ) occurs. The ISR number corresponds to the
 * remapped IRQ vector, typically starting at 32 (e.g., IRQ0 = ISR32).
 *
 * These interrupts originate from external devices and are routed through
 * the IOAPIC to the local APIC. All hardware IRQs provide an error code
 * of zero by convention.
 *
 * @param isrnumber The remapped interrupt vector number (≥ 32).
 * @param errorcode Always zero for hardware IRQs.
 */
void IRQ(uint64_t isrnumber, uint64_t errorcode);

/**
 * @brief Deregister an interrupt handler for a specific ISR or IRQ.
 *
 * Removes the first matching handler function for the given interrupt vector.
 * If the interrupt vector corresponds to an IRQ (n ≥ 32) and no more handlers remain,
 * the interrupt is masked again using the IOAPIC.
 *
 * @param n The interrupt vector number (0–255). Values ≥ 32 represent IRQs.
 * @param handler The handler function to deregister.
 * @return true if the handler was found and removed, false if not found.
 */
bool deregister_interrupt_handler(uint8_t n, isr_t handler);

/**
 * @brief Remap legacy IRQs 0–23 to IOAPIC redirection entries.
 *
 * This function configures the IOAPIC to handle all standard IRQs
 * (0 through 23) by setting up redirection table entries with appropriate
 * vector numbers, polarity, and trigger modes. All entries are initially
 * remapped in a masked (disabled) state to prevent spurious interrupts.
 *
 * The actual unmasking of IRQs should be done later by calling
 * `ioapic_mask_set(gsi, false)` once an appropriate handler is installed.
 * Calling `register_interrupt_handler()` does this automatically.
 */
void remap_irqs_to_ioapic(void);
