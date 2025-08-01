/**
 * @file apic.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 * @brief Local APIC (Advanced Programmable Interrupt Controller) interface
 *
 * This header provides low-level routines and register definitions to interface with
 * the Local APIC (LAPIC) on x86_64 systems. It allows for reading and writing LAPIC registers,
 * detecting the current CPU's LAPIC ID, and querying the LAPIC base address.
 *
 * This interface is used during system initialisation to configure per-core interrupt handling,
 * timer setup, and inter-processor signalling on SMP systems.
 */

#pragma once

/** Default virtual memory mapping address for the Local APIC (used by the kernel) */
#define APIC_ADDRESS 0x4000

/** Model Specific Register for the LAPIC base address */
#define APIC_BASE_MSR 0x1B

/** Bitmask to enable the LAPIC via the APIC_BASE_MSR */
#define APIC_BASE_MSR_ENABLE 0x800

/* LAPIC register offsets (relative to the LAPIC base address) */
#define APIC_ID        0x0020  /**< LAPIC ID Register (read-only) */
#define APIC_VERSION   0x0030  /**< LAPIC Version Register (read-only) */

/**
 * @brief Read a Local APIC register.
 *
 * Works in both xAPIC and x2APIC modes:
 * - In xAPIC mode, the register is memory‑mapped at the LAPIC base address.
 * - In x2APIC mode, registers are accessed via MSRs at 0x800 + (reg >> 4).
 *
 * @param reg Register offset (e.g. APIC_EOI = 0xB0).
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
 * @param reg Register offset (e.g. APIC_EOI = 0xB0).
 * @param val Value to write.
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

void boot_aps();