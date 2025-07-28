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
 * @brief Read a 32-bit value from a LAPIC register
 *
 * @param reg Register offset from LAPIC base
 * @return uint32_t The value read from the register
 */
uint32_t apic_read(uint64_t reg);

/**
 * @brief Write a 32-bit value to a LAPIC register
 *
 * @param reg Register offset from LAPIC base
 * @param value The value to write
 */
void apic_write(uint64_t reg, uint32_t value);

/**
 * @brief Get the Local APIC ID of the current CPU
 *
 * @return uint8_t APIC ID read from the LAPIC ID register.
 * This value uniquely identifies the core in an SMP system and
 * may be non-contiguous or BIOS-assigned.
 */
uint8_t cpu_id();


/**
 * @brief Get the physical address of the Local APIC
 *
 * @return uint64_t Physical address of the LAPIC MMIO region
 */
uint64_t get_lapic_address();

