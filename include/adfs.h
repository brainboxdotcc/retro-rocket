/**
 * @file adfs.h
 * @brief ADFS S/M/L read-only filesystem support
 *
 * Old map format with small directories
 * Uses 256 byte sectors
 */

#pragma once
#include <kernel.h>

#define ADFS_SECTOR_SIZE 256

#define ADFS_DIR_MARKER "Hugo"
#define ADFS_DIR_ENTRIES 47

/**
 * ADFS old map sector
 *
 * Sector 0 and 1 contain free space map and disc info
 */
typedef struct __attribute__((packed)) adfs_map_sector_t {
	uint8_t data[ADFS_SECTOR_SIZE];
} adfs_map_sector_t;

typedef struct __attribute__((packed)) adfs_dir_header_t {
	uint8_t sequence;
	char marker[4];
} adfs_dir_header_t;

typedef struct __attribute__((packed)) adfs_dir_entry_t {
	char name[10];
	uint32_t load_addr;
	uint32_t exec_addr;
	uint32_t length;
	uint8_t start[3];
	uint8_t sequence;
} adfs_dir_entry_t;

typedef struct __attribute__((packed)) adfs_dir_tail_t {
	uint8_t last_mark;          /**< 0 marks end of entries */
	char dir_name[10];          /**< directory name */
	uint8_t parent[3];          /**< parent disc address */
	char title[19];             /**< directory title */
	uint8_t reserved[14];
	uint8_t end_master_sequence; /**< must match header */
	char end_name[4];           /**< "Hugo" or "Nick" */
	uint8_t check_byte;         /**< directory check byte */
} adfs_dir_tail_t;

/**
 * ADFS extent (derived from map)
 *
 * Old map stores free space, so extents are derived at mount
 */
typedef struct adfs_extent_t {
	uint32_t start;
	uint32_t length;
} adfs_extent_t;

/**
 * Mounted ADFS volume
 */
typedef struct adfs_t {
	storage_device_t *device;

	uint32_t total_sectors;

	adfs_extent_t *extents;
	size_t extent_count;

	struct adfs_t *next;
} adfs_t;

void init_adfs();

const char* init_ramdisk_from_adfs_image(uint8_t *image, size_t length);