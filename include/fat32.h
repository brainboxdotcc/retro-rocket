#pragma once

#include "kernel.h"

#define FAT32_SIGNATURE		0x41615252

#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_LONG_NAME		0x0F

/**
 * @brief FAT32 FSINFO structure
 */
typedef struct fat32_fs_info_t {
	uint32_t signature1;
	char reserved1[480];
	uint32_t structsig;
	uint32_t freecount;
	uint32_t nextfree;
	char reserved2[12];
	uint32_t trailsig;
} __attribute__((packed)) fat32_fs_info_t;

/**
 * @brief FAT32 filesystem information, used internally
 * by the driver. This is a higher level version of FSINFO
 */
typedef struct fat32_t {
	char device_name[16];
	uint8_t partitionid;
	char* volume_name;
	uint32_t start;
	uint32_t length;
	uint32_t rootdircluster;
	uint16_t reservedsectors;
	uint32_t fsinfocluster;
	uint8_t numberoffats;
	uint32_t fatsize;
	uint32_t clustersize;
	uint32_t* fat;
	fs_directory_entry_t* root;
	fat32_fs_info_t* info;
} fat32_t;

/**
 * @brief A directory entry, e.g. file, volume label, directory,
 * long filename entry.
 */
typedef struct directory_entry_t {
	char name[11];
	uint8_t attr;
	uint8_t nt;
	uint8_t create_time_tenths;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t first_cluster_hi;
	uint16_t write_time;
	uint16_t write_date;
	uint16_t first_cluster_lo;
	uint32_t size;
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
	uint16_t bytespersector;
	uint8_t sectorspercluster;
	uint16_t reservedsectors;
	uint8_t numberoffats;
	uint16_t numberofdirentries;
	uint16_t totalsectors;
	uint8_t mediatype;
	uint16_t unusedsectorsperfat;
	uint16_t sectorspertrack;
	uint16_t numberofheads;
	uint32_t hiddensectors;
	uint32_t sectorsonmedia;
	// Extended Boot Record	
	uint32_t sectorsperfat;
	uint16_t flags;
	uint16_t fatversion;
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
