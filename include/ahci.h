/**
 * @file ahci.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

typedef enum ahci_fis_type_t {
	FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} ahci_fis_type_t;

typedef struct ahci_fis_reg_h2d_t {
	uint8_t fis_type;		// FIS_TYPE_REG_H2D
	uint8_t pmport:4;		// Port multiplier
	uint8_t rsv0:3;			// Reserved
	uint8_t command_or_control:1;	// 1: Command, 0: Control
	uint8_t command;		// Command register
	uint8_t feature_low;		// Feature register, 7:0
	uint8_t lba0;			// LBA low register, 7:0
	uint8_t lba1;			// LBA mid register, 15:8
	uint8_t lba2;			// LBA 	high register, 23:16
	uint8_t device;			// Device register
	uint8_t lba3;			// LBA register, 31:24
	uint8_t lba4;			// LBA register, 39:32
	uint8_t lba5;			// LBA register, 47:40
	uint8_t feature_high;		// Feature register, 15:8
	uint8_t count_low;		// Count register, 7:0
	uint8_t count_high;		// Count register, 15:8
	uint8_t icc;			// Isochronous command completion
	uint8_t control;		// Control register
 	uint8_t rsv1[4];		// Reserved
} __attribute__((packed)) ahci_fis_reg_h2d_t;

typedef struct ahci_fis_reg_d2h_t {
	uint8_t fis_type;	// FIS_TYPE_REG_D2H
 	uint8_t pmport:4;	// Port multiplier
	uint8_t rsv0:2;		// Reserved
	uint8_t interrupt:1;	// Interrupt bit
	uint8_t rsv1:1;		// Reserved
	uint8_t status;		// Status register
	uint8_t error;		// Error register
	uint8_t lba0;		// LBA low register, 7:0
	uint8_t lba1;		// LBA mid register, 15:8
	uint8_t lba2;		// LBA high register, 23:16
	uint8_t device;		// Device register
	uint8_t lba3;		// LBA register, 31:24
	uint8_t lba4;		// LBA register, 39:32
	uint8_t lba5;		// LBA register, 47:40
	uint8_t rsv2;		// Reserved
	uint8_t count_low;	// Count register, 7:0
	uint8_t count_high;	// Count register, 15:8
	uint8_t rsv3[2];	// Reserved
	uint8_t rsv4[4];	// Reserved
} __attribute__((packed)) ahci_fis_reg_d2h_t;

typedef struct ahci_fis_data_t {
	uint8_t fis_type;	// FIS_TYPE_DATA
	uint8_t pmport:4;	// Port multiplier
	uint8_t rsv0:4;		// Reserved
	uint8_t rsv1[2];	// Reserved
	uint32_t data[];	// Payload
} __attribute__((packed)) ahci_fis_data_t;

typedef struct ahci_fis_pio_setup_t {
	uint8_t fis_type;		// FIS_TYPE_PIO_SETUP
	uint8_t pmport:4;		// Port multiplier
	uint8_t rsv0:1;			// Reserved
	uint8_t direction:1;		// Data transfer direction, 1 - device to host
	uint8_t interrupt:1;		// Interrupt bit
	uint8_t rsv1:1;
	uint8_t status;			// Status register
	uint8_t error;			// Error register
	uint8_t lba0;			// LBA low register, 7:0
	uint8_t lba1;			// LBA mid register, 15:8
	uint8_t lba2;			// LBA high register, 23:16
	uint8_t device;			// Device register
	uint8_t lba3;			// LBA register, 31:24
	uint8_t lba4;			// LBA register, 39:32
	uint8_t lba5;			// LBA register, 47:40
	uint8_t rsv2;			// Reserved
	uint8_t count_low;		// Count register, 7:0
	uint8_t count_high;		// Count register, 15:8
	uint8_t rsv3;			// Reserved
	uint8_t e_status;		// New value of status register
	uint16_t transfer_count;	// Transfer count
	uint8_t rsv4[2];		// Reserved
} __attribute__((packed)) ahci_fis_pio_setup_t;

typedef struct ahci_fis_dma_setup_t {
	uint8_t fis_type;		// FIS_TYPE_DMA_SETUP
	uint8_t pmport:4;		// Port multiplier
	uint8_t rsv0:1;			// Reserved
	uint8_t direction:1;		// Data transfer direction, 1 - device to host
	uint8_t interrupt:1;		// Interrupt bit
	uint8_t activate:1;		// Auto-activate. Specifies if DMA Activate FIS is needed
        uint8_t rsved[2];		// Reserved
        uint64_t dma_buffer_id;		// DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
        uint32_t rsvd;			//More reserved
        uint32_t dma_buffer_offset;	//Byte offset into buffer. First 2 bits must be 0
        uint32_t transfer_count;	//Number of bytes to transfer. Bit 0 must be 0
        uint32_t resvd;			//Reserved
 
} __attribute__((packed)) ahci_fis_dma_setup_t;

typedef volatile struct ahci_hba_port_t {
	uint32_t command_list_base_addr_lower;	// 0x00, command list base address, 1K-byte aligned
	uint32_t command_list_base_addr_upper;	// 0x04, command list base address upper 32 bits
	uint32_t fis_base_addr_lower;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fis_base_addr_upper;		// 0x0C, FIS base address upper 32 bits
	uint32_t interrupt_status;		// 0x10, interrupt status
	uint32_t interrupt_enable;		// 0x14, interrupt enable
	uint32_t cmd;				// 0x18, command and status
	uint32_t rsv0;				// 0x1C, Reserved
	uint32_t task_file_data;		// 0x20, task file data
	uint32_t signature;			// 0x24, signature
	uint32_t sata_status;			// 0x28, SATA status (SCR0:SStatus)
	uint32_t sata_control;			// 0x2C, SATA control (SCR2:SControl)
	uint32_t sata_error;			// 0x30, SATA error (SCR1:SError)
	uint32_t sata_active;			// 0x34, SATA active (SCR3:SActive)
	uint32_t command_issue;			// 0x38, command issue
	uint32_t sata_notification; 		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fis_switch_control;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];			// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];			// 0x70 ~ 0x7F, vendor specific
} __attribute__((packed)) ahci_hba_port_t;

typedef volatile struct ahci_hba_mem_t {
	// 0x00 - 0x2B, Generic Host Control
	uint32_t host_capabilities;		// 0x00, Host capability
	uint32_t global_host_control;		// 0x04, Global host control
	uint32_t interrupt_status;		// 0x08, Interrupt status
	uint32_t ports_implemented; 		// 0x0C, Ports implemented bitmask
	uint32_t version;			// 0x10, Version
	uint32_t ccc_ctl;			// 0x14, Command completion coalescing control
	uint32_t ccc_pts;			// 0x18, Command completion coalescing ports
	uint32_t em_loc;			// 0x1C, Enclosure management location
	uint32_t em_ctl;			// 0x20, Enclosure management control
	uint32_t extended_host_capabilities;	// 0x24, Host capabilities extended
	uint32_t bios_handoff_control;		// 0x28, BIOS/OS handoff control and status
	uint8_t rsv[0xA0-0x2C];			// Reserved
	uint8_t vendor[0x100-0xA0];		// Vendor specific
	ahci_hba_port_t	ports[32];		// Ports 0-31 depending on ports_implemented bits
} __attribute__((packed)) ahci_hba_mem_t;
 
typedef struct ahci_fis_dev_bits_t {
	uint8_t fis_type;
	uint8_t pmport:4;
	uint8_t rsvd:2;
	uint8_t interrupt:1;
	uint8_t notification:1;
	uint8_t status_low:3;
	uint8_t rsvd2:1;
	uint8_t status_high:3;
	uint8_t rsvd3:1;
	uint8_t error;
} __attribute__((packed)) ahci_fis_dev_bits_t;

typedef volatile struct achi_hba_fis_t {
	ahci_fis_dma_setup_t dma_setup_fis;	// DMA Setup FIS
	uint8_t pad0[4];
	ahci_fis_pio_setup_t pio_setup_fis;	// PIO Setup FIS
	uint8_t pad1[12];
	ahci_fis_reg_d2h_t register_fis;	// Register – Device to Host FIS
	uint8_t pad2[4];
	ahci_fis_dev_bits_t set_device_bit_fis;	// Set Device Bit FIS
 	uint8_t ufis[64];
 	uint8_t rsv[0x100-0xA0];
} __attribute__((packed)) achi_hba_fis_t;

typedef struct ahci_hba_cmd_header_t {
	uint8_t command_fis_length:5;		// Command FIS length in DWORDS, 2 ~ 16
	uint8_t atapi:1;			// ATAPI
	uint8_t write:1;			// Write, 1: H2D, 0: D2H
	uint8_t prefetchable:1;		// Prefetchable
	uint8_t reset:1;			// Reset
	uint8_t built_in_self_test:1;		// BIST
	uint8_t clear_busy_on_ok:1;		// Clear busy upon R_OK
	uint8_t rsv0:1;			// Reserved
	uint8_t port_multiplier_port:4;	// Port multiplier port
 	uint16_t prdt_length_entries;		// Physical region descriptor table length in entries
	volatile uint32_t prdbc; 		// Physical region descriptor byte count transferred
	uint32_t command_table_base_lower;	// Command table descriptor base address
	uint32_t command_table_base_upper;	// Command table descriptor base address upper 32 bits
	uint32_t rsv1[4];			// Reserved
} __attribute__((packed)) ahci_hba_cmd_header_t;

typedef struct ahci_hba_prdt_entry_t {
	uint32_t data_address_lower;	// Data base address
	uint32_t data_address_upper;	// Data base address upper 32 bits
	uint32_t rsv0;			// Reserved
	uint32_t byte_count:22;		// Byte count, 4M max
	uint32_t rsv1:9;		// Reserved
	uint32_t interrupt:1;		// Interrupt on completion
} __attribute__((packed)) ahci_hba_prdt_entry_t;

typedef struct achi_hba_cmd_tbl_t {
	uint8_t command_fis[64];		// Command FIS
	uint8_t atapi_command[16];		// ATAPI command, 12 or 16 bytes
	uint8_t rsv[48];			// Reserved
	ahci_hba_prdt_entry_t prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} __attribute__((packed)) achi_hba_cmd_tbl_t;


#define ATA_IDENT_DEVICETYPE		0
#define ATA_IDENT_CYLINDERS		2
#define ATA_IDENT_HEADS			6
#define ATA_IDENT_SECTORS		12
#define ATA_IDENT_SERIAL		20
#define ATA_IDENT_MODEL			54
#define ATA_IDENT_CAPABILITIES		98
#define ATA_IDENT_FIELDVALID		106
#define ATA_IDENT_MAX_LBA		120
#define ATA_IDENT_COMMANDSETS		164
#define ATA_IDENT_MAX_LBA_EXT		200

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier
 
#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4
 
#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

#define HBA_PxIS_TFES (1 << 30)
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define HDD_SECTOR_SIZE 512
#define ATAPI_SECTOR_SIZE 2048

struct storage_device_t;

#define GET_SLOT(port, abar) int slot = find_cmdslot(port, abar); if (slot == -1) { return false; }

void init_ahci();

int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar);
bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);
bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar);
void build_sata_label(struct storage_device_t *sd, const uint8_t *id_page);
void humanise_capacity(char *out, size_t out_len, uint64_t bytes);
int check_type(ahci_hba_port_t* port);
int storage_device_ahci_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer);
int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer);