/**
 * @file ahci.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief AHCI/SATA register and FIS definitions plus driver-facing prototypes.
 * @copyright Copyright (c) 2012-2025
 *
 * @note Bitfield layout in these structs matches x86_64 GCC/Clang ABI and AHCI 1.3.x diagrams.
 *       Bitfield layout is implementation-defined by C; if porting to other ABIs/endianness,
 *       prefer explicit masking on byte arrays.
 */
#pragma once

#include <kernel.h>
#include <scsi.h>

/**
 * @brief Frame Information Structure (FIS) type identifiers.
 */
typedef enum ahci_fis_type_t {
	FIS_TYPE_REG_H2D	= 0x27,	/**< Register FIS - host to device */
	FIS_TYPE_REG_D2H	= 0x34,	/**< Register FIS - device to host */
	FIS_TYPE_DMA_ACT	= 0x39,	/**< DMA activate FIS - device to host */
	FIS_TYPE_DMA_SETUP	= 0x41,	/**< DMA setup FIS - bidirectional */
	FIS_TYPE_DATA		= 0x46,	/**< Data FIS - bidirectional */
	FIS_TYPE_BIST		= 0x58,	/**< BIST activate FIS - bidirectional */
	FIS_TYPE_PIO_SETUP	= 0x5F,	/**< PIO setup FIS - device to host */
	FIS_TYPE_DEV_BITS	= 0xA1,	/**< Set device bits FIS - device to host */
} ahci_fis_type_t;

/**
 * @brief Register FIS: Host to Device (H2D).
 */
typedef struct ahci_fis_reg_h2d_t {
	uint8_t fis_type;		/**< FIS type (FIS_TYPE_REG_H2D) */
	uint8_t pmport:4;		/**< Port multiplier port */
	uint8_t rsv0:3;			/**< Reserved, must be zero */
	uint8_t command_or_control:1;	/**< 1: Command, 0: Control */
	uint8_t command;		/**< Command register */
	uint8_t feature_low;		/**< Feature register, bits 7:0 */
	uint8_t lba0;			/**< LBA low register, bits 7:0 */
	uint8_t lba1;			/**< LBA mid register, bits 15:8 */
	uint8_t lba2;			/**< LBA high register, bits 23:16 */
	uint8_t device;			/**< Device register */
	uint8_t lba3;			/**< LBA register, bits 31:24 */
	uint8_t lba4;			/**< LBA register, bits 39:32 */
	uint8_t lba5;			/**< LBA register, bits 47:40 */
	uint8_t feature_high;		/**< Feature register, bits 15:8 */
	uint8_t count_low;		/**< Count register, bits 7:0 */
	uint8_t count_high;		/**< Count register, bits 15:8 */
	uint8_t icc;			/**< Isochronous command completion */
	uint8_t control;		/**< Control register */
	uint8_t rsv1[4];		/**< Reserved, must be zero */
} __attribute__((packed)) ahci_fis_reg_h2d_t;

/**
 * @brief Register FIS: Device to Host (D2H).
 */
typedef struct ahci_fis_reg_d2h_t {
	uint8_t fis_type;	/**< FIS type (FIS_TYPE_REG_D2H) */
	uint8_t pmport:4;	/**< Port multiplier port */
	uint8_t rsv0:2;		/**< Reserved, must be zero */
	uint8_t interrupt:1;	/**< Interrupt bit (I) */
	uint8_t rsv1:1;		/**< Reserved, must be zero */
	uint8_t status;		/**< ATA Status register snapshot */
	uint8_t error;		/**< ATA Error register snapshot */
	uint8_t lba0;		/**< LBA low register, bits 7:0 */
	uint8_t lba1;		/**< LBA mid register, bits 15:8 */
	uint8_t lba2;		/**< LBA high register, bits 23:16 */
	uint8_t device;		/**< Device register */
	uint8_t lba3;		/**< LBA register, bits 31:24 */
	uint8_t lba4;		/**< LBA register, bits 39:32 */
	uint8_t lba5;		/**< LBA register, bits 47:40 */
	uint8_t rsv2;		/**< Reserved, must be zero */
	uint8_t count_low;	/**< Count register, bits 7:0 */
	uint8_t count_high;	/**< Count register, bits 15:8 */
	uint8_t rsv3[2];	/**< Reserved, must be zero */
	uint8_t rsv4[4];	/**< Reserved, must be zero */
} __attribute__((packed)) ahci_fis_reg_d2h_t;

/**
 * @brief Data FIS (bidirectional payload).
 */
typedef struct ahci_fis_data_t {
	uint8_t fis_type;	/**< FIS type (FIS_TYPE_DATA) */
	uint8_t pmport:4;	/**< Port multiplier port */
	uint8_t rsv0:4;		/**< Reserved, must be zero */
	uint8_t rsv1[2];	/**< Reserved, must be zero */
	uint32_t data[];	/**< Dword-aligned payload */
} __attribute__((packed)) ahci_fis_data_t;

/**
 * @brief PIO Setup FIS (device to host).
 */
typedef struct ahci_fis_pio_setup_t {
	uint8_t fis_type;		/**< FIS type (FIS_TYPE_PIO_SETUP) */
	uint8_t pmport:4;		/**< Port multiplier port */
	uint8_t rsv0:1;			/**< Reserved, must be zero */
	uint8_t direction:1;		/**< Data direction: 1 = device->host */
	uint8_t interrupt:1;		/**< Interrupt bit (I) */
	uint8_t rsv1:1;			/**< Reserved, must be zero */
	uint8_t status;			/**< Status register */
	uint8_t error;			/**< Error register */
	uint8_t lba0;			/**< LBA low, bits 7:0 */
	uint8_t lba1;			/**< LBA mid, bits 15:8 */
	uint8_t lba2;			/**< LBA high, bits 23:16 */
	uint8_t device;			/**< Device register */
	uint8_t lba3;			/**< LBA, bits 31:24 */
	uint8_t lba4;			/**< LBA, bits 39:32 */
	uint8_t lba5;			/**< LBA, bits 47:40 */
	uint8_t rsv2;			/**< Reserved, must be zero */
	uint8_t count_low;		/**< Count, bits 7:0 */
	uint8_t count_high;		/**< Count, bits 15:8 */
	uint8_t rsv3;			/**< Reserved, must be zero */
	uint8_t e_status;		/**< New Status value */
	uint16_t transfer_count;	/**< Transfer count (bytes) */
	uint8_t rsv4[2];		/**< Reserved, must be zero */
} __attribute__((packed)) ahci_fis_pio_setup_t;

/**
 * @brief DMA Setup FIS (bidirectional).
 * @note dma_buffer_offset low 2 bits must be 0 (DWORD aligned).
 * @note transfer_count bit 0 must be 0 (even byte count).
 */
typedef struct ahci_fis_dma_setup_t {
	uint8_t fis_type;		/**< FIS type (FIS_TYPE_DMA_SETUP) */
	uint8_t pmport:4;		/**< Port multiplier port */
	uint8_t rsv0:1;			/**< Reserved, must be zero */
	uint8_t direction:1;		/**< Data direction: 1 = device->host */
	uint8_t interrupt:1;		/**< Interrupt bit (I) */
	uint8_t activate:1;		/**< Auto-activate (DMA Activate FIS needed) */
	uint8_t rsved[2];		/**< Reserved, must be zero */
	uint64_t dma_buffer_id;		/**< Host DMA buffer identifier (host-specific) */
	uint32_t rsvd;			/**< Reserved, must be zero */
	uint32_t dma_buffer_offset;	/**< Byte offset into buffer (bits[1:0]==0) */
	uint32_t transfer_count;	/**< Bytes to transfer (bit0==0) */
	uint32_t resvd;			/**< Reserved, must be zero */

} __attribute__((packed)) ahci_fis_dma_setup_t;

/**
 * @brief AHCI Port register block (Px*).
 * @note Addresses labelled "base address" require the documented alignment (1 KiB for CLB, 256 B for FB).
 */
typedef volatile struct ahci_hba_port_t {
	uint32_t command_list_base_addr_lower;	/**< 0x00, Command List Base Address (CLB), 1 KiB aligned */
	uint32_t command_list_base_addr_upper;	/**< 0x04, CLB upper 32 bits */
	uint32_t fis_base_addr_lower;		/**< 0x08, FIS Base Address (FB), 256 B aligned */
	uint32_t fis_base_addr_upper;		/**< 0x0C, FB upper 32 bits */
	uint32_t interrupt_status;		/**< 0x10, Interrupt Status (PxIS) */
	uint32_t interrupt_enable;		/**< 0x14, Interrupt Enable (PxIE) */
	uint32_t cmd;				/**< 0x18, Command and Status (PxCMD) */
	uint32_t rsv0;				/**< 0x1C, Reserved */
	uint32_t task_file_data;		/**< 0x20, Task File Data (PxTFD) */
	uint32_t signature;			/**< 0x24, Signature (PxSIG) */
	uint32_t sata_status;			/**< 0x28, SATA Status (SCR0:SStatus) */
	uint32_t sata_control;			/**< 0x2C, SATA Control (SCR2:SControl) */
	uint32_t sata_error;			/**< 0x30, SATA Error (SCR1:SError) */
	uint32_t sata_active;			/**< 0x34, SATA Active (SCR3:SActive) */
	uint32_t command_issue;			/**< 0x38, Command Issue (PxCI) */
	uint32_t sata_notification; 		/**< 0x3C, SATA Notification (SCR4:SNotification) */
	uint32_t fis_switch_control;		/**< 0x40, FIS-based Switch Control (PxFBS) */
	uint32_t rsv1[11];			/**< 0x44–0x6F, Reserved */
	uint32_t vendor[4];			/**< 0x70–0x7F, Vendor specific */
} __attribute__((packed)) ahci_hba_port_t;

/**
 * @brief AHCI Host Bus Adapter (HBA) memory-mapped register space.
 */
typedef volatile struct ahci_hba_mem_t {
	/* 0x00 - 0x2B, Generic Host Control */
	uint32_t host_capabilities;		/**< 0x00, Host Capabilities (CAP) */
	uint32_t global_host_control;		/**< 0x04, Global Host Control (GHC) */
	uint32_t interrupt_status;		/**< 0x08, Interrupt Status (IS) */
	uint32_t ports_implemented; 		/**< 0x0C, Ports Implemented bitmask (PI) */
	uint32_t version;			/**< 0x10, Version (VS) */
	uint32_t ccc_ctl;			/**< 0x14, Command Completion Coalescing Control */
	uint32_t ccc_pts;			/**< 0x18, Command Completion Coalescing Ports */
	uint32_t em_loc;			/**< 0x1C, Enclosure Management Location */
	uint32_t em_ctl;			/**< 0x20, Enclosure Management Control */
	uint32_t extended_host_capabilities;	/**< 0x24, Extended Host Capabilities (CAP2) */
	uint32_t bios_handoff_control;		/**< 0x28, BIOS/OS Handoff Control and Status (BOHC) */
	uint8_t rsv[0xA0-0x2C];			/**< 0x2C–0x9F, Reserved */
	uint8_t vendor[0x100-0xA0];		/**< 0xA0–0xFF, Vendor specific */
	ahci_hba_port_t	ports[32];		/**< 0x100–, Port control registers (Px) */
} __attribute__((packed)) ahci_hba_mem_t;

/**
 * @brief Set Device Bits (SDB) FIS (device->host asynchronous notification).
 * @note Reconstruct the full ATA Status as (status_low | (status_high << 3)).
 */
typedef struct ahci_fis_dev_bits_t {
	uint8_t fis_type;		/**< FIS type (FIS_TYPE_DEV_BITS) */
	uint8_t pmport:4;		/**< Port multiplier port */
	uint8_t rsvd:2;		/**< Reserved, must be zero */
	uint8_t interrupt:1;		/**< Interrupt bit (I) */
	uint8_t notification:1;		/**< Notification (N) */
	uint8_t status_low:3;		/**< Status bits [2:0] */
	uint8_t rsvd2:1;		/**< Reserved, must be zero */
	uint8_t status_high:3;		/**< Status bits [7:5] (packed MSBs) */
	uint8_t rsvd3:1;		/**< Reserved, must be zero */
	uint8_t error;			/**< Error register (valid if ERR set) */
} __attribute__((packed)) ahci_fis_dev_bits_t;

/**
 * @brief Received FIS area layout in system memory (per-port).
 */
typedef volatile struct ahci_hba_fis_t {
	ahci_fis_dma_setup_t dma_setup_fis;	/**< 0x00–0x1F, DMA Setup FIS */
	uint8_t pad0[4];			/**< Padding */
	ahci_fis_pio_setup_t pio_setup_fis;	/**< 0x20–0x3F, PIO Setup FIS */
	uint8_t pad1[12];			/**< Padding */
	ahci_fis_reg_d2h_t register_fis;	/**< 0x40–0x57, Register – Device to Host FIS */
	uint8_t pad2[4];			/**< Padding */
	ahci_fis_dev_bits_t set_device_bit_fis;	/**< 0x58–0x5F, Set Device Bits FIS */
	uint8_t ufis[64];			/**< 0x60–0x9F, Unknown/other FIS (vendor/ATAPI) */
	uint8_t rsv[0x100-0xA0];		/**< 0xA0–0xFF, Reserved */
} __attribute__((packed)) ahci_hba_fis_t;

/**
 * @brief Command Header (one per command slot).
 * @note command_fis_length is measured in DWORDs (2–16).
 * @note prdbc is written by HBA with bytes actually transferred.
 */
typedef struct ahci_hba_cmd_header_t {
	uint8_t command_fis_length:5;		/**< Command FIS length (DWORDs, 2–16) */
	uint8_t atapi:1;			/**< ATAPI command (1 = ATAPI) */
	uint8_t write:1;			/**< Direction: 1 = H2D write, 0 = D2H read */
	uint8_t prefetchable:1;			/**< Prefetchable hint */
	uint8_t reset:1;			/**< Device reset */
	uint8_t built_in_self_test:1;		/**< BIST */
	uint8_t clear_busy_on_ok:1;		/**< Clear busy upon R_OK */
	uint8_t rsv0:1;				/**< Reserved, must be zero */
	uint8_t port_multiplier_port:4;		/**< Port multiplier port */
	uint16_t prdt_length_entries;		/**< PRDT length in entries */
	volatile uint32_t prdbc; 		/**< Bytes transferred by HBA (read-only to host) */
	uint32_t command_table_base_lower;	/**< Command Table Base Address (CTBA) */
	uint32_t command_table_base_upper;	/**< CTBA upper 32 bits */
	uint32_t rsv1[4];			/**< Reserved, must be zero */
} __attribute__((packed)) ahci_hba_cmd_header_t;

/**
 * @brief Physical Region Descriptor Table (PRDT) entry.
 * @note byte_count is encoded as (N - 1); N must be even; N ≤ 4 MiB.
 */
typedef struct ahci_hba_prdt_entry_t {
	uint32_t data_address_lower;	/**< Data base address (lower) */
	uint32_t data_address_upper;	/**< Data base address (upper) */
	uint32_t rsv0;			/**< Reserved, must be zero */
	uint32_t byte_count:22;		/**< Byte count minus 1 (max 4 MiB - 1) */
	uint32_t rsv1:9;		/**< Reserved, must be zero */
	uint32_t interrupt:1;		/**< Interrupt on completion (IOC) */
} __attribute__((packed)) ahci_hba_prdt_entry_t;

/**
 * @brief Command Table (points to CFIS/ACMD and PRDT).
 */
typedef struct ahci_hba_cmd_tbl_t {
	uint8_t command_fis[64];		/**< Command FIS (CFIS) */
	uint8_t atapi_command[16];		/**< ATAPI packet command (12 or 16 bytes) */
	uint8_t rsv[48];			/**< Reserved, must be zero */
	ahci_hba_prdt_entry_t prdt_entry[1];	/**< PRDT entries (0–65535, flexible array) */
} __attribute__((packed)) ahci_hba_cmd_tbl_t;

#define ATA_CMD_DATA_SET_MANAGEMENT 0x06

struct ahci_trim_range {
	uint64_t lba;      /* 48-bit used */
	uint32_t sectors;  /* 512-byte sectors */
};

/* IDENTIFY DEVICE words of interest:
 *  - w169 bit0: DATA SET MANAGEMENT / TRIM supported
 *  - w69  bit14: Deterministic read after TRIM (DRAT)
 *  - w69  bit5 : Read zeroes after TRIM (RZAT)
 *  - w105     : Max 512B DSM parameter blocks per command (0 => 1)
 */

typedef struct {
	bool has_trim;
	bool drat;
	bool rzat;
	uint16_t max_dsm_blocks; /* 512-byte blocks; 0 => 1 */
} ahci_trim_caps;

/* ----------------------------- ATA identify offsets ----------------------------- */

/** @brief ATA IDENTIFY: device type offset (bytes). */
#define ATA_IDENT_DEVICETYPE		0
/** @brief ATA IDENTIFY: cylinder count offset (bytes). */
#define ATA_IDENT_CYLINDERS		2
/** @brief ATA IDENTIFY: head count offset (bytes). */
#define ATA_IDENT_HEADS			6
/** @brief ATA IDENTIFY: sectors per track offset (bytes). */
#define ATA_IDENT_SECTORS		12
/** @brief ATA IDENTIFY: serial number string offset (bytes). */
#define ATA_IDENT_SERIAL		20
/** @brief ATA IDENTIFY: model string offset (bytes). */
#define ATA_IDENT_MODEL			54
/** @brief ATA IDENTIFY: capabilities word offset (bytes). */
#define ATA_IDENT_CAPABILITIES		98
/** @brief ATA IDENTIFY: field validity word offset (bytes). */
#define ATA_IDENT_FIELDVALID		106
/** @brief ATA IDENTIFY: 28-bit max LBA (bytes). */
#define ATA_IDENT_MAX_LBA		120
/** @brief ATA IDENTIFY: command sets supported offset (bytes). */
#define ATA_IDENT_COMMANDSETS		164
/** @brief ATA IDENTIFY: 48-bit max LBA (bytes). */
#define ATA_IDENT_MAX_LBA_EXT		200

/* ----------------------------- SATA signatures ----------------------------- */

/** @brief SATA device signature: ATA drive. */
#define	SATA_SIG_ATA	0x00000101
/** @brief SATA device signature: ATAPI (SATAPI) device. */
#define	SATA_SIG_ATAPI	0xEB140101
/** @brief SATA device signature: Enclosure Management Bridge. */
#define	SATA_SIG_SEMB	0xC33C0101
/** @brief SATA device signature: Port Multiplier. */
#define	SATA_SIG_PM	0x96690101

#define HBA_CAP_S64A (1u << 31) /* 64-bit Addressing (AHCI CAP.S64A) */

/* ----------------------------- Device type codes ----------------------------- */

/** @brief AHCI enumerated device type: none. */
#define AHCI_DEV_NULL 0
/** @brief AHCI enumerated device type: SATA. */
#define AHCI_DEV_SATA 1
/** @brief AHCI enumerated device type: SEMB. */
#define AHCI_DEV_SEMB 2
/** @brief AHCI enumerated device type: Port Multiplier. */
#define AHCI_DEV_PM 3
/** @brief AHCI enumerated device type: SATAPI. */
#define AHCI_DEV_SATAPI 4

/* ----------------------------- Link/phy states ----------------------------- */

/** @brief Interface Power Management (IPM) state: active. */
#define HBA_PORT_IPM_ACTIVE 1
/** @brief Device Detection (DET) state: device present. */
#define HBA_PORT_DET_PRESENT 3

/* ----------------------------- PxCMD bits ----------------------------- */

/** @brief PxCMD: Start (ST). */
#define HBA_PxCMD_ST    0x0001
/** @brief PxCMD: FIS Receive Enable (FRE). */
#define HBA_PxCMD_FRE   0x0010
/** @brief PxCMD: FIS Receive Running (FR) (read-only). */
#define HBA_PxCMD_FR    0x4000
/** @brief PxCMD: Command List Running (CR) (read-only). */
#define HBA_PxCMD_CR    0x8000

/* ----------------------------- Interrupt/status and ATA opcodes ----------------------------- */

#define HBA_PxIS_TFES	(1 << 30)   /** Task file error */
#define HBA_PxIS_IFS	(1u << 27)  /* Interface fatal error */
#define HBA_PxIS_HBDS	(1u << 28)  /* Host bus data error */
#define HBA_PxIS_HBFS	(1u << 29)  /* Host bus fatal error */

/** @brief ATA command: READ DMA EXT (48-bit). */
#define ATA_CMD_READ_DMA_EX 0x25
/** @brief ATA command: WRITE DMA EXT (48-bit). */
#define ATA_CMD_WRITE_DMA_EX 0x35

/* ----------------------------- ATA Status bits ----------------------------- */

/** @brief ATA Status: Busy (BSY). */
#define ATA_DEV_BUSY 0x80
/** @brief ATA Status: Data Request (DRQ). */
#define ATA_DEV_DRQ 0x08

/* ----------------------------- Sector sizes ----------------------------- */

/** @brief Logical HDD sector size in bytes (common 512 B). */
#define HDD_SECTOR_SIZE 512
/** @brief ATAPI logical sector size in bytes (2048 B). */
#define ATAPI_SECTOR_SIZE 2048

/* ----------------------------- Forward decls ----------------------------- */

/** @brief Opaque storage device descriptor used by higher layers. */
struct storage_device_t;

/* ----------------------------- Helper macro ----------------------------- */

/**
 * @brief Acquire a free command slot or return false from the caller.
 * @param port AHCI port register block.
 * @param abar AHCI HBA memory block.
 * @note Expands to a declaration `int slot` and early-return path on failure.
 *       Intended for use at the start of functions that submit commands.
 */
#define GET_SLOT(port, abar) int slot = find_cmdslot(port, abar); if (slot == -1) { return false; }

/* ----------------------------- Public API ----------------------------- */

/**
 * @brief Initialise AHCI controller(s), enumerate ports, and register devices.
 */
void init_ahci();

/**
 * @brief Find a free command slot on a port.
 * @param port AHCI port register block.
 * @param abar AHCI HBA memory block.
 * @return Slot index [0..31] on success, -1 if none available.
 */
int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar);

/**
 * @brief Read from a SATA block device via DMA.
 * @param port AHCI port register block.
 * @param start Starting LBA.
 * @param count Number of sectors to read.
 * @param buf Destination buffer (caller-managed).
 * @param abar AHCI HBA memory block.
 * @return true on success, false on error.
 */
bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);

/**
 * @brief Write to a SATA block device via DMA.
 * @param port AHCI port register block.
 * @param start Starting LBA.
 * @param count Number of sectors to write.
 * @param buf Source buffer (caller-managed).
 * @param abar AHCI HBA memory block.
 * @return true on success, false on error.
 */
bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);

/**
 * @brief Read from an ATAPI device (e.g., optical) via PACKET command.
 * @param port AHCI port register block.
 * @param start Starting logical block address (ATAPI).
 * @param count Number of logical blocks to read.
 * @param buf Destination buffer (caller-managed).
 * @param abar AHCI HBA memory block.
 * @return true on success, false on error.
 */
bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);

/**
 * @brief Query capacity (in bytes) from a SATA device using IDENTIFY data.
 * @param port AHCI port register block.
 * @param abar AHCI HBA memory block.
 * @return Device size in bytes.
 */
uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar);

/**
 * @brief Build a human-readable SATA device label from IDENTIFY data.
 * @param sd Storage device object to populate.
 * @param id_page 512-byte IDENTIFY DEVICE buffer.
 */
void build_sata_label(struct storage_device_t *sd, const uint8_t *id_page);

/**
 * @brief Render a human-readable capacity string (e.g., "931 GiB").
 * @param out Output buffer.
 * @param out_len Output buffer length.
 * @param bytes Capacity in bytes.
 */
void humanise_capacity(char *out, size_t out_len, uint64_t bytes);

/**
 * @brief Inspect PxSIG/PxSSTS to determine attached device type.
 * @param port AHCI port register block.
 * @return One of AHCI_DEV_* constants.
 */
int check_type(ahci_hba_port_t* port);

/**
 * @brief Block device read shim for the storage layer (bytes interface).
 * @param dev Opaque device pointer (struct storage_device_t*).
 * @param start Byte offset to read from (converted to sectors internally).
 * @param bytes Number of bytes to read.
 * @param buffer Destination buffer.
 * @return 0 on success, negative errno-style on failure.
 */
int storage_device_ahci_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer);

/**
 * @brief Block device write shim for the storage layer (bytes interface).
 * @param dev Opaque device pointer (struct storage_device_t*).
 * @param start Byte offset to write to (converted to sectors internally).
 * @param bytes Number of bytes to write.
 * @param buffer Source buffer.
 * @return 0 on success, negative errno-style on failure.
 */
int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer);

/**
 * @brief Wait until the AHCI port and attached device are ready to accept a new command.
 * @param port AHCI HBA port.
 * @return true if ready, false on timeout or fatal port state.
 */
bool wait_for_ready(ahci_hba_port_t* port);

/**
 * @brief Fill one PRDT entry in the command table.
 * @param cmdtbl Command table to modify.
 * @param index PRDT entry index to fill.
 * @param address Physical or DMA-able address of the data buffer.
 * @param byte_count Number of bytes to transfer (1..4MiB per entry, device-dependent).
 * @param interrupt Set true to request interrupt on completion for this PRDT entry.
 */
void fill_prdt(ahci_hba_cmd_tbl_t* cmdtbl, size_t index, void* address, uint32_t byte_count, bool interrupt);

/**
 * @brief Obtain and initialise a command header for a slot.
 * @param port AHCI HBA port.
 * @param slot Command slot number (0..31).
 * @param write Set true for host-to-device (write), false for device-to-host (read).
 * @param atapi Set true for ATAPI packet command, false for ATA.
 * @param prdtls Number of PRDT entries to advertise (PRDTL).
 * @return Pointer to the prepared command header.
 */
ahci_hba_cmd_header_t* get_cmdheader_for_slot(ahci_hba_port_t* port, size_t slot, bool write, bool atapi, uint16_t prdtls);

/**
 * @brief Return the command table for a header and clear CFIS/ATAPI/reserved regions.
 * @param cmdheader Command header whose table will be accessed.
 * @return Pointer to the command table.
 */
ahci_hba_cmd_tbl_t* get_and_clear_cmdtbl(ahci_hba_cmd_header_t* cmdheader);

/**
 * @brief Prepare a Register H2D FIS in the command table.
 * @param cmdtbl Command table to place the FIS into.
 * @param type FIS type (e.g., FIS_TYPE_REG_H2D).
 * @param command ATA/ATAPI command opcode.
 * @param feature_low Feature low byte.
 * @return Pointer to the FIS within the command table.
 */
ahci_fis_reg_h2d_t* setup_reg_h2d(ahci_hba_cmd_tbl_t* cmdtbl, uint8_t type, uint8_t command, uint8_t feature_low);

/**
 * @brief Fill LBA and sector count fields in a Register H2D FIS.
 * @param cmdfis Pointer to the FIS previously created by setup_reg_h2d.
 * @param start Starting LBA.
 * @param count Sector count (device logical block units).
 */
void fill_reg_h2c(ahci_fis_reg_h2d_t* cmdfis, uint64_t start, uint16_t count);

/**
 * @brief Issue a prepared command in the given slot by setting PxCI.
 * @param port AHCI HBA port.
 * @param slot Command slot number (0..31).
 */
void issue_command_to_slot(ahci_hba_port_t *port, uint8_t slot);

/**
 * @brief Wait for a command slot to complete and detect transport/taskfile errors.
 * @param port AHCI HBA port.
 * @param slot Command slot number (0..31).
 * @param function Short label used for diagnostics/logging.
 * @return true on success, false if a taskfile/transport error occurred (error already logged).
 */
bool wait_for_completion(ahci_hba_port_t* port, uint8_t slot, const char* function);

/**
 * @brief Convenience wrapper to issue a command and wait for completion.
 * @param port AHCI HBA port.
 * @param slot Command slot number (0..31).
 * @param function Short label used for diagnostics/logging.
 * @return true on success, false on error (error already logged).
 */
bool issue_and_wait(ahci_hba_port_t* port, uint8_t slot, const char* function);

/**
 * @brief Trim trailing ASCII spaces from a NUL-terminated string in place.
 * @param s String buffer to modify.
 */
void trim_trailing_spaces(char *s);

/**
 * @brief Build a human-readable ATAPI device label from INQUIRY data.
 * @param sd Storage device object to receive the label.
 * @param inq 36+ byte SCSI INQUIRY response buffer.
 */
void build_atapi_label(struct storage_device_t *sd, const uint8_t *inq);

/**
 * @brief Eject removable media from an ATAPI device (ALLOW removal then START STOP with LOEJ).
 * @param port AHCI HBA port for the device.
 * @param abar AHCI HBA memory (ABAR) pointer.
 * @return true on command acceptance, false on error (caller may fetch sense).
 */
bool atapi_eject(ahci_hba_port_t *port, ahci_hba_mem_t *abar);

/**
 * @brief Issue IDENTIFY (ATA or ATAPI) and copy the 512-byte identify page to out.
 * @param port AHCI HBA port.
 * @param abar AHCI HBA memory (ABAR) pointer.
 * @param out Destination buffer for identify data (512 bytes).
 * @return true on success, false on error.
 */
bool ahci_identify_page(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out);

/**
 * @brief Issue ATAPI INQUIRY (0x12) and copy the response.
 * @param port AHCI HBA port.
 * @param abar AHCI HBA memory (ABAR) pointer.
 * @param out Destination buffer for INQUIRY data.
 * @param len Allocation length requested and number of bytes to copy.
 * @return true on success, false on error (caller may use sense mapping).
 */
bool atapi_enquiry(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out, uint8_t len);

/**
 * @brief Map AHCI/AHCI-ATA/ATAPI latched status on a port to a fixed fs_error_t without performing I/O.
 * @param port AHCI HBA port; uses PxIS/PxSERR/PxTFD/PxSIG snapshots.
 * @return fs_error_t value describing the failure class.
 */
int ahci_classify_error(ahci_hba_port_t* port);

/**
 * @brief Map SCSI/ATAPI sense (sense key, ASC, ASCQ) to a fixed fs_error_t.
 * @param sense_key SCSI sense key.
 * @param additional_sense_code Additional Sense Code (ASC).
 * @param additional_sense_code_qualifier Additional Sense Code Qualifier (ASCQ).
 * @return fs_error_t value describing the failure class.
 */
int scsi_map_sense_to_fs_error(scsi_sense_key_t sense_key, scsi_additional_sense_code_t additional_sense_code, scsi_additional_sense_code_qualifier_t additional_sense_code_qualifier);

/**
 * @brief Issue ATAPI REQUEST SENSE (6) and copy sense data to the caller buffer.
 * @param port AHCI HBA port.
 * @param abar AHCI HBA memory (ABAR) pointer.
 * @param out Destination buffer for sense data.
 * @param out_len Allocation length requested (typically 18).
 * @return true on success, false on failure to obtain sense data.
 */
bool atapi_request_sense6(ahci_hba_port_t* port, ahci_hba_mem_t* abar, uint8_t* out, uint8_t out_len);

/**
 * @brief Extract sense key, ASC and ASCQ from a fixed-format (0x70/0x71) REQUEST SENSE buffer.
 * @param buf Sense buffer (>= 14 bytes, typically 18).
 * @param sense_key Output: sense key.
 * @param additional_sense_code Output: ASC.
 * @param additional_sense_code_qualifier Output: ASCQ.
 */
void scsi_extract_fixed_sense(const uint8_t* buf, scsi_sense_key_t* sense_key, scsi_additional_sense_code_t* additional_sense_code, scsi_additional_sense_code_qualifier_t* additional_sense_code_qualifier);

/**
 * @brief Handle ATAPI CHECK CONDITION by issuing REQUEST SENSE, mapping to fs_error_t, logging a single line, and returning false.
 * @param port AHCI HBA port.
 * @param abar AHCI HBA memory (ABAR) pointer.
 * @param function Short label used for diagnostics/logging.
 * @return Always false to simplify call-sites that branch on failure.
 */
bool atapi_handle_check_condition(ahci_hba_port_t* port, ahci_hba_mem_t* abar, const char* function);


/**
 * @brief Map AHCI/AHCI-ATA/ATAPI status to a fixed fs_error_t.
 *
 * Uses PxIS priority: HBFS → HBDS → IFS → TFES. For TFES, inspects PxTFD ERR bits.
 * For ATAPI, returns FS_ERR_ATAPI_CHECK on TFES so the caller can issue REQUEST SENSE.
 *
 * This function performs no I/O; it only interprets the latched registers you pass in.
 */
int ahci_classify_error(ahci_hba_port_t* port);

/**
 * @brief Map SCSI/ATAPI sense (SK/ASC/ASCQ) to fs_error_t with fixed message.
 */
int scsi_map_sense_to_fs_error(scsi_sense_key_t sense_key, scsi_additional_sense_code_t additional_sense_code, scsi_additional_sense_code_qualifier_t additional_sense_code_qualifier);

/**
 * @brief Issue ATAPI REQUEST SENSE (6) and copy sense data to 'out'.
 * Expects 'out_len' >= 18 for standard fixed-format sense.
 * Returns true on success (command accepted and data DMA'd), false on failure.
 */
bool atapi_request_sense6(ahci_hba_port_t* port, ahci_hba_mem_t* abar, uint8_t* out, uint8_t out_len);

/**
 * @brief Extract SK/ASC/ASCQ from a fixed-format (0x70/0x71) REQUEST SENSE buffer.
 * Caller guarantees 'buf' points to at least 14 bytes (we usually pass 18).
 */
void scsi_extract_fixed_sense(const uint8_t* buf, scsi_sense_key_t* sense_key, scsi_additional_sense_code_t* additional_sense_code, scsi_additional_sense_code_qualifier_t* additional_sense_code_qualifier);

/**
 * @brief Convenience helper for ATAPI CHECK CONDITION paths.
 * Issues REQUEST SENSE, extracts and maps sense, logs a single line, then returns false.
 *
 * Always returns false so call-sites can simply: if (!atapi_handle_check_condition(...)) return false;
 * If REQUEST SENSE itself fails, logs a generic timeout/hardware error.
 */
bool atapi_handle_check_condition(ahci_hba_port_t* port, ahci_hba_mem_t* abar, const char* function);

bool ahci_trim_one_range(ahci_hba_port_t *port, ahci_hba_mem_t *abar, const ahci_trim_caps *caps, uint64_t lba, uint32_t sectors);

ahci_trim_caps ahci_probe_trim_caps(const uint16_t *id_words);

bool storage_device_ahci_block_clear(void *dev, uint64_t lba, uint32_t sectors);

/* ----------------------------- Layout sanity checks ----------------------------- */

/** @brief Compile-time size check: command header must be 0x20 bytes. */
_Static_assert(sizeof(ahci_hba_cmd_header_t) == 0x20, "cmd header size");
/** @brief Compile-time offset check: PxCI must be at 0x38. */
_Static_assert(offsetof(ahci_hba_port_t, command_issue) == 0x38, "CI offset");
/** @brief Compile-time offset check: SDB FIS must be at 0x58 within received FIS area. */
_Static_assert(offsetof(ahci_hba_fis_t, set_device_bit_fis) == 0x58, "SDB FIS offset");
