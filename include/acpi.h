/**
 * @file acpi.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
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