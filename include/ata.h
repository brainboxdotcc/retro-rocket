/**
 * @file ata.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#define		ATA_CMD_PACKET		0xA0
#define		ATA_CMD_IDENTIFY	0xEC

#define		ATAPI_CMD_READ		0xA8
#define		ATA_IDENT_MAX_LBA	120
#define		ATA_IDENT_MAX_LBA_EXT	200

void init_ide();
