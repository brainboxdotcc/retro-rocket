/**
 * @file apic.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 * @brief Local APIC (Advanced Programmable Interrupt Controller) interface
 *
 * This header provides low-level routines and register definitions to interface with
 * the Local APIC (LAPIC) on x86_64 systems. It allows for reading and writing LAPIC registers,
 * detecting the current CPU's LAPIC ID, sending inter-processor interrupts (IPIs),
 * and querying the LAPIC base address.
 *
 * The LAPIC is a per-core interrupt controller responsible for:
 * - Handling local interrupts (e.g., timer, thermal, performance counters).
 * - Providing per-core programmable timer functionality.
 * - Supporting Inter-Processor Interrupts (IPIs) for SMP coordination.
 * - Forwarding external interrupts via the I/O APIC.
 *
 * Both xAPIC (MMIO) and x2APIC (MSR-based) modes are supported by this interface.
 */

#pragma once

/** @brief Default virtual memory mapping address for the Local APIC MMIO page. */
#define APIC_ADDRESS    0x4000

/** @brief Model Specific Register (MSR) index for LAPIC base address (IA32_APIC_BASE). */
#define APIC_BASE_MSR   0x1B

/** @brief Bitmask in IA32_APIC_BASE MSR to enable the Local APIC. */
#define APIC_BASE_MSR_ENABLE   0x800

/* -------------------------------------------------------------------------- */
/* LAPIC Register Offsets (xAPIC mode, relative to MMIO base)                  */
/* -------------------------------------------------------------------------- */

/** @brief Local APIC ID Register (read-only). */
#define APIC_ID         0x0020

/** @brief Local APIC Version Register (read-only). */
#define APIC_VERSION    0x0030

/** @brief Task Priority Register (TPR). Controls interrupt priority threshold. */
#define APIC_TPR        0x0080

/** @brief Interrupt Command Register (ICR), low dword (vector + control flags). */
#define APIC_ICR_LOW    0x0300

/** @brief Interrupt Command Register (ICR), high dword (destination LAPIC ID). */
#define APIC_ICR_HIGH   0x0310

/* -------------------------------------------------------------------------- */
/* Delivery Modes for Inter-Processor Interrupts (IPI)                        */
/* -------------------------------------------------------------------------- */

/** @brief Fixed delivery mode: deliver interrupt using vector field. */
#define APIC_DM_FIXED       0x000

/** @brief Non-Maskable Interrupt (NMI) delivery mode. */
#define APIC_DM_NMI         0x400

/** @brief INIT delivery mode: reset target CPU to INIT state. */
#define APIC_DM_INIT        0x500

/** @brief Startup IPI delivery mode: start target CPU at specified vector. */
#define APIC_DM_STARTUP     0x600

/* -------------------------------------------------------------------------- */
/* x2APIC Registers (MSR-based access only)                                   */
/* -------------------------------------------------------------------------- */

/** @brief MSR index for the x2APIC Interrupt Command Register (ICR). */
#define IA32_X2APIC_ICR     0x830

/* -------------------------------------------------------------------------- */
/* Miscellaneous constants                                                    */
/* -------------------------------------------------------------------------- */

/** @brief Vector number used for custom AP wake-up IPIs. */
#define APIC_WAKE_IPI       240

/** @brief Destination shorthand: no shorthand, use explicit LAPIC ID. */
#define APIC_DEST_NO_SHORTHAND (0 << 18)

/** @brief Destination mode: physical addressing. */
#define APIC_DEST_PHYSICAL  (0 << 11)

/** @brief Assert level for IPI (used to trigger delivery). */
#define APIC_LEVEL_ASSERT   (1 << 14)

/** @brief Edge-triggered delivery mode for IPI. */
#define APIC_TRIGGER_EDGE   (0 << 15)

/* -------------------------------------------------------------------------- */
/* Function prototypes                                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read a Local APIC register.
 *
 * Works in both xAPIC and x2APIC modes:
 * - In xAPIC mode, the register is memory‑mapped at the LAPIC base address.
 * - In x2APIC mode, registers are accessed via MSRs at 0x800 + (reg >> 4).
 *
 * @param reg Register offset (e.g. APIC_TPR = 0x80).
 * @return uint32_t Value read from the register.
 */
uint32_t apic_read(uint64_t reg);

/**
 * @brief Write to a Local APIC register.
 *
 * Works in both xAPIC and x2APIC modes:
 * - In xAPIC mode, the register is memory‑mapped at the LAPIC base address.
 * - In x2APIC mode, registers are written via MSRs at 0x800 + (reg >> 4).
 *
 * @param reg Register offset (e.g. APIC_TPR = 0x80).
 * @param value Value to write.
 */
void apic_write(uint64_t reg, uint32_t value);

/**
 * @brief Get the Local APIC ID of the current CPU.
 *
 * In xAPIC mode this is an 8-bit value read from the LAPIC ID register.
 * In x2APIC mode this is a 32-bit value read from MSR 0x802.
 *
 * @return uint32_t LAPIC ID of the current CPU.
 * @note LAPIC IDs are assigned by hardware/firmware and may be
 *       sparse, non-contiguous, or not zero-based.
 */
uint32_t cpu_id(void);

/**
 * @brief Get the OS-assigned logical CPU ID of the current CPU.
 *
 * Logical CPU IDs are zero-based, contiguous indices used internally
 * by the kernel. They are mapped to LAPIC IDs via cpu_id_mapping[].
 *
 * @return uint8_t Logical CPU ID of the current CPU.
 */
uint8_t logical_cpu_id(void);

/**
 * @brief Get the physical address of the Local APIC MMIO region.
 *
 * This is only valid when xAPIC mode is active. In x2APIC mode,
 * LAPIC registers are accessed via MSRs instead of MMIO.
 *
 * @return uint64_t Physical address of the LAPIC MMIO base.
 */
uint64_t get_lapic_address(void);

/**
 * @brief Determine if x2APIC mode is enabled.
 *
 * Checks both CPUID feature flags and the IA32_APIC_BASE MSR to confirm
 * whether x2APIC is active.
 *
 * @return int Non-zero if x2APIC mode is enabled, zero otherwise.
 */
int x2apic_enabled(void);

/**
 * @brief Read a 64-bit Model Specific Register (MSR).
 *
 * @param msr The MSR address to read.
 * @return uint64_t The value read from the MSR.
 */
uint64_t rdmsr(uint32_t msr);

/**
 * @brief Map a logical CPU ID to its LAPIC ID.
 *
 * @param cpu_id Logical CPU ID (zero-based, kernel-assigned).
 * @return uint32_t LAPIC ID for the given logical CPU ID.
 */
uint32_t get_lapic_id_from_cpu_id(uint8_t cpu_id);

/**
 * @brief Map a LAPIC ID to its logical CPU ID.
 *
 * @param lapic_id LAPIC ID as reported by hardware.
 * @return uint8_t Logical CPU ID assigned to this LAPIC ID,
 *                 or 255 if unmapped.
 */
uint8_t get_cpu_id_from_lapic_id(uint32_t lapic_id);

/**
 * @brief Initialise and bring Application Processors (APs) online via Limine SMP.
 *
 * This function uses the Limine SMP response to discover all CPUs in the system,
 * map their LAPIC IDs to logical CPU IDs, and assign each AP an entry point
 * (`kmain_ap`). The Bootstrap Processor (BSP) is detected and skipped, as are
 * CPUs with processor IDs >= 255 (reserved for broadcast or outside kernel limits).
 *
 * Once all eligible APs are assigned their startup entry point, this function
 * spins until each AP reports online by incrementing @ref aps_online.
 *
 * @note This does not directly issue INIT/SIPI IPIs; Limine handles the actual
 *       startup of APs. The kernel only assigns the entry function and waits.
 * @warning Systems with more than 254 CPUs are truncated to 254 usable cores.
 *
 * @see kmain_ap, aps_online, set_lapic_id_for_cpu_id
 */
void boot_aps();


/**
 * @brief Send an Inter-Processor Interrupt (IPI) to a specific LAPIC ID.
 *
 * Supports both xAPIC (ICR registers) and x2APIC (IA32_X2APIC_ICR MSR).
 *
 * @param lapic_id Destination LAPIC ID.
 * @param vector Interrupt vector to deliver.
 */
void apic_send_ipi(uint32_t lapic_id, uint8_t vector);

/**
 * @brief Send a wake-up IPI to a target logical CPU.
 *
 * This function is used to wake an Application Processor (AP) from a halted or
 * waiting state. It resolves the kernel-assigned logical CPU ID into the
 * hardware LAPIC ID and then sends a fixed-mode IPI using the predefined
 * wake-up vector (APIC_WAKE_IPI).
 *
 * @param logical_cpu_id Logical CPU ID (zero-based, contiguous index assigned
 *                       by the kernel).
 *
 * @note The Local APIC on the target CPU must already be enabled for the wake-up
 *       IPI to be accepted. This function assumes that LAPIC initialisation has
 *       been performed during CPU bring-up.
 */
void wake_cpu(uint8_t logical_cpu_id);

/**
 * @brief Initialise the Local APIC on an Application Processor (AP).
 *
 * This function enables the Local APIC by setting the spurious interrupt
 * vector register's enable bit, and lowers the Task Priority Register (TPR)
 * to zero so that the AP can receive all interrupts.
 *
 * It is typically invoked during AP startup after IDT installation, allowing
 * the processor to handle interrupts and inter-processor IPIs.
 *
 * @note This does not configure LAPIC timers or IPI handlers; it only ensures
 *       that the APIC is enabled and accepting interrupts.
 *
 * @see apic_write
 * @see APIC_SVR
 * @see APIC_TPR
 */
void apic_setup_ap();