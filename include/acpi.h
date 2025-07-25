/**
 * @file acpi.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>
#include <uacpi/uacpi.h>
#include <uacpi/namespace.h>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

struct acpi_driver {
	const char *device_name;
	const char *const *pnp_ids;
	int (*device_probe)(uacpi_namespace_node *node, uacpi_namespace_node_info *info);
	struct acpi_driver *next;
};

void acpi_register_driver(struct acpi_driver *drv);
void acpi_enumerate_devices(void);

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

typedef struct {
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t int_pin;   // 0 = INTA#, 1 = INTB#...
	uint32_t gsi;
	uint16_t flags;    // Polarity and trigger mode
} pci_irq_route_t;

#define MAX_PCI_ROUTES 64

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
