/**
 * @file ata.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#define		ATA_CMD_PACKET		0xA0
#define		ATA_CMD_IDENTIFY	0xEC
#define		ATAPI_CMD_READ10	0x28
#define		ATAPI_CMD_READ12	0xA8
#define		ATAPI_CMD_INQUIRY	0x12
#define		ATA_IDENT_MAX_LBA	120
#define		ATA_IDENT_MAX_LBA_EXT	200

/* ATA Error Register bits */
#define ATA_ERR_AMNF   0x01  /* Address mark not found (obsolete, legacy) */
#define ATA_ERR_TK0NF  0x02  /* Track 0 not found (obsolete, legacy) */
#define ATA_ERR_ABRT   0x04  /* Aborted command (illegal/unsupported) */
#define ATA_ERR_MCR    0x08  /* Media change request (obsolete, legacy) */
#define ATA_ERR_IDNF   0x10  /* ID not found (LBA not found) */
#define ATA_ERR_MC     0x20  /* Media changed */
#define ATA_ERR_UNC    0x40  /* Uncorrectable data error */
#define ATA_ERR_ICRC   0x80  /* Interface CRC error / bad sector transfer */


void init_ide();
