/**
 * @file fat32.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#pragma once

#include "kernel.h"

/**
 * @brief FAT32 FSINFO sector signature
 */
#define FAT32_SIGNATURE		0x41615252
#define FAT32_SIGNATURE2	0x61417272
#define FAT32_SIGNATURE3	0xAA550000

#define ATTR_READ_ONLY		0x01	// Read-only file
#define ATTR_HIDDEN		0x02	// Hidden file
#define ATTR_SYSTEM		0x04	// System file
#define ATTR_VOLUME_ID		0x08	// Volume ID (only applicable in root directory)
#define ATTR_DIRECTORY		0x10	// Directory
#define ATTR_ARCHIVE		0x20	// File ready for archiving
#define ATTR_LONG_NAME		0x0F	// Long name (RO+Hidden+Sys+VID)

#define ATTR_LFN_DELETED	0x80	// Deleted long filename bit flag
#define ATTR_LFN_LAST_ENTRY	0x40	// Long filename last entry bit flag

#define DELETED_ENTRY		0xE5	// Deleted entry if first char of short filename

#define CLUSTER_END		0x0FFFFFF8	// Ending cluster of chain (warning: May also find 0x0FFFFF0 as this value!)
#define CLUSTER_BAD		0x0FFFFFF7	// Bad cluster marker (we treat this as end of chain like CLUSTER_END)
#define CLUSTER_FREE		0x00000000	// Free cluster

/**
 * @brief EFI system partition GUID
 */
#define GPT_EFI_SYSTEM			"28732AC1-1FF8-D211-BA4B-00A0C93EC93B"
/**
 * @brief Microsoft basic data partition GUID
 */
#define GPT_MICROSOFT_BASIC_DATA	"EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"

/**
 * @brief FAT32 FSINFO structure
 */
typedef struct fat32_fs_info_t {
	uint32_t signature1;		// Must contain FAT32_SIGNATURE
	char reserved1[480];		// Should be zeroed (reserved)
	uint32_t structsig;		// Must contain FAT32_SIGNATURE2
	uint32_t freecount;		// Number of free clusters (possibly incorrect)
	uint32_t nextfree;		// Next free cluster number (likely incorrect)
	char reserved2[12];		// Should be zeroed (reserved)
	uint32_t trailsig;		// Must contain FAT32_SIGNATURE3
} __attribute__((packed)) fat32_fs_info_t;

/**
 * @brief FAT32 filesystem information, used internally
 * by the driver. This is a higher level version of FSINFO
 */
typedef struct fat32_t {
	char device_name[16];		// Device name where the FAT32 volume is
	uint8_t partitionid;		// Partition ID or 0xFF for a GPT partition
	char* volume_name;		// Volume name
	uint64_t start;			// Starting sector
	uint64_t length;		// Length in sectors
	uint32_t rootdircluster;	// Cluster number of root dircetory
	uint16_t reservedsectors;	// Number of reserved sectors
	uint32_t fsinfocluster;		// Cluster number of FSINFO
	uint8_t numberoffats;		// Number of FATs
	uint32_t fatsize;		// Size of each FAT
	uint32_t clustersize;		// Size of a cluster
	fat32_fs_info_t* info;		// FSINFO
} fat32_t;

/**
 * @brief A directory entry, e.g. file, volume label, directory,
 * long filename entry.
 */
typedef struct directory_entry_t {
	char name[11];			// Short filename, Space padded e.g. "TEST    TXT" for TEST.TXT
	uint8_t attr;			// Attributes
	uint8_t nt;			// Reserved for NT use
	uint8_t create_time_tenths;	// Creation time tenths of a second
	uint16_t create_time;		// Creation time
	uint16_t create_date;		// Creation date
	uint16_t access_date;		// Access date
	uint16_t first_cluster_hi;	// High 16 bits of first cluster in chain
	uint16_t write_time;		// Write time
	uint16_t write_date;		// Write date
	uint16_t first_cluster_lo;	// Low 16 bits of first cluster in chain
	uint32_t size;			// Size in bytes (max: 4GB)
} __attribute__((packed)) directory_entry_t;

/**
 * @brief FAT32 BIOS parameter block (BPB),
 * stored in the boot sector of the drive.
 */
typedef struct parameter_block_t {
	uint8_t code1; // 0xEB - machine code to jump over BPB
	uint8_t code2; // 0x76
	uint8_t code3; // 0x90
	char oemidentifier[8];
	uint16_t bytespersector;	// Bytes per sector, use storage device blocksize instead
	uint8_t sectorspercluster;	// Sectors per cluster
	uint16_t reservedsectors;	// Reserved sectors
	uint8_t numberoffats;		// Number of FATs
	uint16_t numberofdirentries;	// Number of root directory entries (not used)
	uint16_t totalsectors;		// Total sectors?
	uint8_t mediatype;		// Media type
	uint16_t unusedsectorsperfat;	// Unused sectors per FAT
	uint16_t sectorspertrack;	// Sectors per track (not used)
	uint16_t numberofheads;		// Number of heads (not used)
	uint32_t hiddensectors;		// Hidden sectors (not used)
	uint32_t sectorsonmedia;	// Sectors on media (not used)
	// Extended Boot Record	
	uint32_t sectorsperfat;		// Sectors per FAT
	uint16_t flags;			// Flags
	uint16_t fatversion;		// FAT version (should be 0)
	uint32_t rootdircluster;
	uint16_t fsinfocluster;
	uint16_t backupbootsectorcluster;
	char reserved[12];
	uint8_t drivenumber;
	uint8_t ntflags;
	uint8_t signature; 
	uint32_t serialnumber;
	char volumelabel[11];
	char systemid[9];
} __attribute__((packed)) parameter_block_t;

/**
 * @brief Long filename entry, overlays a directory_entry_t.
 * These entries repeat before a non-lfn entry, each holding
 * up to 13 UCS-2 characters. Note that they are not stored in
 * order, the 'order' attribute indicates which order they
 * go in.
 */
typedef struct lfn_t {
	/**
	 * @brief Note the order is arbitrary. It may have the
	 * value 0, 3, 65, 48... No way to know! To work around
	 * this weirdness, we have an array of lfn_t[256] that
	 * holds all possible entries ordered by the order value,
	 * which we can then iterate once we encounter a non-lfn
	 * to build the name.
	 */
	uint8_t order;
	uint16_t first[5]; // First 5 UCS-2 characters
	uint8_t attributes; // Always 0x0f
	uint8_t entry_type; // always zero for name entries
	uint8_t checksum; // icky checksum
	uint16_t second[6]; // Second 6 UCS-2 characters
	uint16_t reserved;  // always 0
	uint16_t third[2]; // Third 2 UCS-2 characters
} __attribute__((packed)) lfn_t;

/**
 * @brief Register fat32 as a filesystem type
 */
void init_fat32();
