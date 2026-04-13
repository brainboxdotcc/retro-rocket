/**
 * @file idt.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

/**
 * @note IOAPIC support is mandatory
 *
 * Legacy PIC-only configurations are not supported. Interrupt routing
 * must be performed via IOAPIC and LAPIC.
 */
#define USE_IOAPIC

/**
 * @brief IDT descriptor (IDTR)
 *
 * Structure used by the lidt instruction to load the Interrupt Descriptor Table.
 *
 * @note Packed to match hardware-defined layout
 */
typedef struct idt_ptr_t {
	uint16_t limit; /**< Size of the IDT in bytes minus one */
	void* base;     /**< Linear address of the first IDT entry */
} __attribute__((packed)) idt_ptr_t;

/**
 * @brief IDT entry (64-bit gate descriptor)
 *
 * Describes a single interrupt or exception handler entry in the IDT.
 * Used in 64-bit mode for interrupt, trap, and task gates.
 *
 * Layout must match the x86-64 hardware-defined descriptor format.
 */
typedef struct idt_entry_t {
	uint16_t offset_1;        /**< Handler address bits 0..15 */
	uint16_t selector;        /**< Code segment selector in GDT or LDT */
	uint8_t  ist;             /**< Interrupt Stack Table index (bits 0..2), remaining bits zero */
	uint8_t  type_attributes; /**< Gate type, descriptor privilege level, and present flag */
	uint16_t offset_2;        /**< Handler address bits 16..31 */
	uint32_t offset_3;        /**< Handler address bits 32..63 */
	uint32_t zero;            /**< Reserved, must be zero */
} __attribute__((packed)) idt_entry_t;

/**
 * @brief Active 64-bit IDT descriptor
 *
 * Points to the currently loaded IDT used for interrupt dispatch.
 * Loaded via lidt during initialisation and reused by application processors.
 */
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
 * a LAPIC->OS CPU remapping table. This simplifies per‑CPU structures at the
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

/**
 * @brief Load the shared IDT and enable interrupts on an Application Processor (AP).
 *
 * This function loads the global Interrupt Descriptor Table (IDT) pointer into
 * the AP's IDTR register using the `lidt` instruction. After the IDT is loaded,
 * it enables external interrupts via `sti` (wrapped by interrupts_on()).
 *
 * It is typically called during AP startup before entering the scheduler loop,
 * ensuring that the AP can handle interrupts using the same IDT as the BSP.
 *
 * @note All APs share the same IDT (idt64) in this design.
 * @see lidt, interrupts_on
 */
void load_ap_shared_idt();

/**
 * @brief Load interrupt handling for an application processor (AP)
 *
 * Initialises interrupt handling on a secondary CPU. If FRED is enabled,
 * configures FRED for this CPU. Otherwise, loads the shared IDT.
 */
void load_ap_shared_interrupts();

/**
 * @brief Initialise interrupt subsystem
 *
 * Selects the interrupt delivery mechanism. Attempts to initialise FRED
 * if enabled, otherwise falls back to IDT-based interrupt handling.
 */
void init_interrupts();

/**
 * @brief Early BSP interrupt initialisation
 *
 * Performs early setup of interrupt handlers on the bootstrap processor.
 * Registers core handlers required before full interrupt routing is configured.
 */
void interrupt_bsp_common_early_init(void);

/**
 * @brief Program the PIT timer
 *
 * Configures the Programmable Interval Timer (PIT) to generate periodic
 * interrupts at a fixed frequency.
 */
void interrupt_bsp_program_pit(void);

/**
 * @brief Route hardware IRQs on the BSP
 *
 * Configures interrupt routing for hardware IRQs. Depending on configuration,
 * this either remaps and enables the legacy PIC or disables it and sets up
 * IOAPIC and LAPIC-based routing.
 */
void interrupt_bsp_route_irqs(void);

/**
 * @brief Output active interrupt mechanism
 *
 * Prints the currently selected interrupt delivery mechanism to the screen
 *
 * @param mechanism Human-readable name of the mechanism (e.g. "IDT", "FRED")
 */
void output_interrupt_mechanism(const char* mechanism);

/**
 * @brief Late BSP interrupt initialisation
 *
 * Finalises interrupt setup on the bootstrap processor. Claims deferred IRQs
 * and enables interrupts globally.
 */
void interrupt_bsp_common_late_init(void);
