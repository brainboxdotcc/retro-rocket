/**
 * @file acpi.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

/**
 * @brief ACPI Root System Description Pointer (RSDP)
 */
typedef struct rsdp_t {
	char signature[8];		// "RSD PTR "
	uint8_t checksum;
	char oem_id[6];			// OEM identifier string
	uint8_t revision;		// 0 for ACPI 1.0, >0 for ACPI 2.0 or later
	uint32_t rsdt_address;		// Root System Description Table Address
	uint32_t length;		// Length of Root System Description Table
	uint64_t xsdt_address;		// Extended System Description Table Address
	uint8_t extended_checksum;	// Extended System Description Table Checksum
	uint8_t reserved[3];		// Reserved
} __attribute__((packed)) rsdp_t;

/**
 * @brief System Description Table Header
 */
typedef struct sdt_header_t {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) sdt_header_t;

/**
 * @brief Root System Description Table
 */
typedef struct rsdt_t {
	sdt_header_t header;			// System Description Table header
	uint32_t pointer_to_other_sdt[];	// 32-Bit Pointer to other table

} __attribute__((packed)) rsdt_t;

/**
 * @brief Definition of an IOAPIC
 */
typedef struct ioapic_t {
	uint8_t id;		// The IO APIC id.
	uint64_t paddr;		// The physical address of the MMIO region.
	uint32_t gsi_base;	// The GSI base.
	uint8_t gsi_count;	// The interrupt count.
} ioapic_t;

typedef struct {
	uint8_t type;
	uint8_t length;
	uint8_t bus_source;
	uint8_t irq_source;
	uint32_t gsi;
	uint16_t flags;
} __attribute__((packed)) madt_override_t;

#define IRQ_DEFAULT_POLARITY 0 // active high
#define IRQ_DEFAULT_TRIGGER  0 // edge

typedef enum detected_from {
	FROM_MADT,
	FROM_PRT,
	FROM_FALLBACK,
} detected_from_t;

typedef struct pci_irq_route {
	bool exists;
	uint8_t int_pin;
	uint32_t gsi;
	int polarity; // 0 = high,  1 = low
	int trigger; // 0 = edge, 1 = level
	detected_from_t detected_from;
} pci_irq_route_t;

#define MAX_PCI_ROUTES 256

/**
 * @brief Detect SMP cores, IOAPICs, Local APICs
 */
void init_cores();

/**
 * @brief Get the local apic ids
 * 
 * @return uint8_t* 
 */
uint8_t* get_lapic_ids();

/**
 * @brief Get the cpu count
 * 
 * @return uint16_t 
 */
uint16_t get_cpu_count();

/**
 * @brief Get the local apic
 * 
 * @return uint64_t 
 */
uint64_t get_local_apic();

/**
 * @brief Get the ioapic
 * 
 * @param index IOAPIC index
 * @return ioapic_t 
 */
ioapic_t get_ioapic(uint16_t index);

/**
 * @brief Get the ioapic count
 * 
 * @return uint16_t total number of IOAPICs
 */
uint16_t get_ioapic_count();

uint32_t irq_to_gsi(uint8_t irq);

uint8_t get_irq_polarity(uint8_t irq);

uint8_t get_irq_trigger_mode(uint8_t irq);

const char *triggering_str(uint8_t trig);

const char *polarity_str(uint8_t pol);

const char *sharing_str(uint8_t share);
