#pragma once

#include <kernel.h>

struct rsdp {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__ ((packed));
typedef struct rsdp rsdp_t;

struct sdt_header {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__ ((packed));
typedef struct sdt_header sdt_header_t;


struct rsdt {
	sdt_header_t header;
	uint32_t pointer_to_other_sdt[];

} __attribute__ ((packed));
typedef struct rsdt rsdt_t;

struct ioapic {
	uint8_t id;				 // The IO APIC's id.
	uint64_t paddr;			 // The physical address of the MMIO region.
	uint32_t gsi_base;		  // The GSI base.
	uint8_t gsi_count;		  // The interrupt count.
};
typedef struct ioapic ioapic_t;

void detect_cores();
uint8_t* get_lapic_ids();
uint16_t get_cpu_count();
uint64_t get_local_apic();
ioapic_t get_ioapic(uint16_t index);
uint16_t get_ioapic_count();