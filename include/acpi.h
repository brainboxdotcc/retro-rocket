/**
 * @file acpi.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief ACPI detection and enumeration header. Provides low-level interfaces
 *        to extract CPU topology, IOAPIC mappings, and PCI IRQ routing from
 *        ACPI tables such as RSDP, XSDT, MADT, and others.
 * @copyright Copyright (c) 2012–2025
 */
#pragma once

#include <kernel.h>

/**
 * @brief ACPI Root System Description Pointer (RSDP)
 *
 * This structure serves as the entry point to the ACPI system description tables.
 * Its location is typically found by scanning the BIOS memory range. It may point
 * to either the RSDT (ACPI 1.0) or XSDT (ACPI 2.0+).
 */
typedef struct rsdp_t {
	char signature[8];            ///< Should contain "RSD PTR "
	uint8_t checksum;             ///< Checksum of first 20 bytes
	char oem_id[6];               ///< OEM identifier string
	uint8_t revision;             ///< Revision: 0 = ACPI 1.0, 2+ = ACPI 2.0+
	uint32_t rsdt_address;        ///< 32-bit physical address of RSDT
	uint32_t length;              ///< Total length of the RSDP (ACPI 2.0+)
	uint64_t xsdt_address;        ///< 64-bit physical address of XSDT (ACPI 2.0+)
	uint8_t extended_checksum;    ///< Checksum of entire RSDP (ACPI 2.0+)
	uint8_t reserved[3];          ///< Reserved bytes (must be zero)
} __attribute__((packed)) rsdp_t;

/**
 * @brief Generic ACPI System Description Table Header
 *
 * All ACPI tables begin with this header. It is used to identify table type,
 * length, version, and source information.
 */
typedef struct sdt_header_t {
	char signature[4];            ///< Table identifier (e.g. "APIC", "DSDT", etc.)
	uint32_t length;              ///< Total length of the table, including this header
	uint8_t revision;             ///< Table version
	uint8_t checksum;             ///< Entire table checksum
	char oem_id[6];               ///< OEM identifier string
	char oem_table_id[8];         ///< OEM-defined table identifier
	uint32_t oem_revision;        ///< OEM table revision
	uint32_t creator_id;          ///< Vendor ID of utility that created the table
	uint32_t creator_revision;    ///< Version of utility that created the table
} __attribute__((packed)) sdt_header_t;

/**
 * @brief Root System Description Table (RSDT)
 *
 * Points to other system description tables using 32-bit addresses.
 * Superseded by XSDT in ACPI 2.0+ which uses 64-bit addresses.
 */
typedef struct rsdt_t {
	sdt_header_t header;             ///< Standard ACPI header
	uint32_t pointer_to_other_sdt[]; ///< Array of 32-bit physical addresses to other tables
} __attribute__((packed)) rsdt_t;

/**
 * @brief Describes a detected IOAPIC from the MADT table.
 */
typedef struct ioapic_t {
	uint8_t id;               ///< IOAPIC ID as reported by MADT
	uint64_t paddr;           ///< Physical address of the IOAPIC's MMIO region
	uint32_t gsi_base;        ///< Global System Interrupt (GSI) base for this IOAPIC
	uint8_t gsi_count;        ///< Number of GSIs this IOAPIC handles
} ioapic_t;

/**
 * @brief Describes an IRQ override from the MADT table.
 *
 * Used to override the default IRQ to GSI mapping, and specify trigger mode and polarity.
 */
typedef struct {
	uint8_t type;             ///< Entry type (always 2 for overrides)
	uint8_t length;           ///< Length of this entry
	uint8_t bus_source;       ///< Typically 0 (ISA bus)
	uint8_t irq_source;       ///< Original IRQ number
	uint32_t gsi;             ///< New GSI mapping
	uint16_t flags;           ///< Bitmask defining polarity and trigger mode
} __attribute__((packed)) madt_override_t;

/**
 * @brief Default polarity: active high
 */
#define IRQ_DEFAULT_POLARITY 0

/**
 * @brief Default trigger mode: edge-triggered
 */
#define IRQ_DEFAULT_TRIGGER  0

/**
 * @brief Returned from @ref get_cpu_id_from_lapic_id if the CPUid
 * passed in does not map to a logical CPU id.
 */
#define INVALID_CPU_ID 255

/**
 * @brief Indicates the source of a PCI IRQ route.
 */
typedef enum detected_from {
	FROM_MADT,     ///< IRQ derived from MADT (ACPI interrupt overrides)
	FROM_PRT,      ///< IRQ derived from PCI Routing Table (_PRT)
	FROM_FALLBACK, ///< No override detected, fallback to default IRQ mapping
} detected_from_t;

/**
 * @brief A mapping of PCI interrupt pin to global system interrupt (GSI).
 *
 * Used for routing PCI IRQs through ACPI, based on _PRT or MADT.
 */
typedef struct pci_irq_route {
	bool exists;                ///< True if this route is defined
	uint8_t int_pin;            ///< PCI interrupt pin (INTA–INTD)
	uint32_t gsi;               ///< Global System Interrupt (GSI) to which the pin maps
	int polarity;               ///< Interrupt polarity (0 = high, 1 = low)
	int trigger;                ///< Trigger mode (0 = edge, 1 = level)
	detected_from_t detected_from; ///< Source of route (MADT, PRT, or fallback)
} pci_irq_route_t;

#define MAX_PCI_ROUTES 256 ///< Maximum number of PCI IRQ routes supported

/**
 * @brief Detect SMP CPU cores and APICs (Local and IOAPIC).
 *
 * This function parses the MADT (APIC table) and sets up CPU topology and IOAPIC data.
 */
void init_acpi();

/**
 * @brief Retrieve the IOAPIC structure by index.
 *
 * @param index IOAPIC index in internal array
 * @return ioapic_t structure containing details of the IOAPIC
 */
ioapic_t get_ioapic(uint16_t index);

/**
 * @brief Get the total number of IOAPICs detected in the system.
 *
 * @return Number of IOAPICs.
 */
uint16_t get_ioapic_count();

/**
 * @brief Translate a legacy IRQ number (0–15) into a GSI using overrides.
 *
 * @param irq Legacy IRQ number
 * @return Global System Interrupt (GSI) for the IRQ
 */
uint32_t irq_to_gsi(uint8_t irq);

/**
 * @brief Determine the interrupt polarity of a given IRQ.
 *
 * @param irq IRQ number
 * @return 0 for active high, 1 for active low
 */
uint8_t get_irq_polarity(uint8_t irq);

/**
 * @brief Determine the trigger mode of a given IRQ.
 *
 * @param irq IRQ number
 * @return 0 for edge-triggered, 1 for level-triggered
 */
uint8_t get_irq_trigger_mode(uint8_t irq);

/**
 * @brief Return human-readable string for trigger mode.
 *
 * @param trig Trigger mode value
 * @return "edge" or "level"
 */
const char *triggering_str(uint8_t trig);

/**
 * @brief Return human-readable string for polarity.
 *
 * @param pol Polarity value
 * @return "high" or "low"
 */
const char *polarity_str(uint8_t pol);

/**
 * @brief Return human-readable string for IRQ sharing model.
 *
 * @param share Sharing type
 * @return Descriptive string of the IRQ sharing status
 */
const char *sharing_str(uint8_t share);

/**
 * Main entrypoint for an SMP AP
 * @param info limine CPU info
 */
void kmain_ap(struct limine_smp_info *info);