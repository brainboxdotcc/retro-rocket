#ifndef __ATA_H__
#define __ATA_H__

#include <stdint.h>

#define    ATA_SR_BSY   0x80
#define    ATA_SR_DRDY  0x40
#define    ATA_SR_DF    0x20
#define    ATA_SR_DSC   0x10
#define    ATA_SR_DRQ   0x08
#define    ATA_SR_CORR  0x04
#define    ATA_SR_IDX   0x02
#define    ATA_SR_ERR   0x01

#define    ATA_ER_BBK   0x80
#define    ATA_ER_UNC   0x40
#define    ATA_ER_MC    0x20
#define    ATA_ER_IDNF  0x10
#define    ATA_ER_MCR   0x08
#define    ATA_ER_ABRT  0x04
#define    ATA_ER_TK0NF 0x02
#define    ATA_ER_AMNF  0x01

// ATA-Commands:
#define      ATA_CMD_READ_PIO         0x20
#define      ATA_CMD_READ_PIO_EXT     0x24
#define      ATA_CMD_READ_DMA         0xC8
#define      ATA_CMD_READ_DMA_EXT     0x25
#define      ATA_CMD_WRITE_PIO        0x30
#define      ATA_CMD_WRITE_PIO_EXT    0x34
#define      ATA_CMD_WRITE_DMA        0xCA
#define      ATA_CMD_WRITE_DMA_EXT    0x35
#define      ATA_CMD_CACHE_FLUSH      0xE7
#define      ATA_CMD_CACHE_FLUSH_EXT  0xEA
#define      ATA_CMD_PACKET           0xA0
#define      ATA_CMD_IDENTIFY_PACKET  0xA1
#define      ATA_CMD_IDENTIFY         0xEC

#define      ATAPI_CMD_READ   0xA8
#define      ATAPI_CMD_EJECT  0x1B

#define    ATA_IDENT_DEVICETYPE   0
#define    ATA_IDENT_CYLINDERS    2
#define    ATA_IDENT_HEADS        6
#define    ATA_IDENT_SECTORS      12
#define    ATA_IDENT_SERIAL       20
#define    ATA_IDENT_MODEL        54
#define    ATA_IDENT_CAPABILITIES 98
#define    ATA_IDENT_FIELDVALID   106
#define    ATA_IDENT_MAX_LBA      120
#define    ATA_IDENT_COMMANDSETS  164
#define    ATA_IDENT_MAX_LBA_EXT  200

#define      ATA_MASTER      0x00
#define      ATA_SLAVE      0x01

#define      IDE_ATA         0x00
#define      IDE_ATAPI      0x01

// ATA-ATAPI Task-File:
#define      ATA_REG_DATA       0x00
#define      ATA_REG_ERROR      0x01
#define      ATA_REG_FEATURES   0x01
#define      ATA_REG_SECCOUNT0  0x02
#define      ATA_REG_LBA0       0x03
#define      ATA_REG_LBA1       0x04
#define      ATA_REG_LBA2       0x05
#define      ATA_REG_HDDEVSEL   0x06
#define      ATA_REG_COMMAND    0x07
#define      ATA_REG_STATUS     0x07
#define      ATA_REG_SECCOUNT1  0x08
#define      ATA_REG_LBA3       0x09
#define      ATA_REG_LBA4       0x0A
#define      ATA_REG_LBA5       0x0B
#define      ATA_REG_CONTROL    0x0C
#define      ATA_REG_ALTSTATUS  0x0C
#define      ATA_REG_DEVADDRESS 0x0D

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

// Directions
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01

enum ata_poll_error_code
{
	ATA_POLL_NO_ERROR = 0,
	ATA_POLL_DEVICE_FAULT = 1,
	ATA_POLL_GENRAL_ERROR = 2,
	ATA_POLL_DRQ_NOT_SET = 3,
	ATA_POLL_TIMEOUT = 4,
};

typedef struct
{
	uint16_t base;  // I/O Base.
	uint16_t ctrl;  // Control Base
	uint16_t bmide; // Bus Master IDE
	uint8_t  nIEN;  // nIEN (No Interrupt)
} channel;

typedef struct {
	uint8_t  reserved;     // 0 (Empty) or 1 (This Drive really exists).
	uint8_t  channel;      // 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t  drive;        // 0 (Master Drive) or 1 (Slave Drive).
	uint16_t type;         // 0: ATA, 1:ATAPI.
	uint16_t sign;         // Drive Signature
	uint16_t capabilities; // Features.
	uint32_t commandsets;  // Command Sets Supported.
	uint64_t size;         // Size in Sectors.
	uint8_t  model[41];    // Model in string.
} ide_device;

uint8_t ide_read(uint8_t channel, uint8_t reg);
void ide_write(uint8_t channel, uint8_t reg, uint8_t data);
void ide_initialise();
void ide_irq(uint8_t isr, uint64_t error, uint64_t irq);
int ide_read_sectors(uint8_t drive, uint16_t numsects, uint64_t lba, uint64_t buffer_address);
int ide_write_sectors(uint8_t drive, uint16_t numsects, uint64_t lba, uint64_t buffer_address);
int ide_atapi_eject(uint8_t drive);

#endif

