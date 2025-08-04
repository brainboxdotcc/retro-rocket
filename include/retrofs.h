/**
 * @file retrofs.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Structures and functions for RetroFS, an ADFS L format inspired
 * file system for Retro Rocket.
 * @copyright Copyright (c) 2012–2025
 */
#pragma once

#include <kernel.h>

#define RFS_SECTOR_SIZE 512
#define RFS_MAX_NAME 128
#define RFS_FS_MAP_BITS_PER_SECTOR (512 / sizeof(uint64_t))

/**
 * @brief Default directory size of a block in sectors
 * (rfs_directory_start_t::sectors)
 * 32 sectors holds 63 files (the first entry is the start
 * entry that contains the continuation pointer etc)
 */
#define RFS_DEFAULT_DIR_SIZE	32

#define RFS_FLAG_DIRECTORY	0x0001
#define RFS_FLAG_LOCKED		0x0002
#define RFS_FLAG_DIR_START	0x0004

#define RFS_ID			(uint64_t)0x3153466f72746552ULL // "RetroFS1"

/**
 * @brief RetroFS GPT Partition Type GUID
 * UUID v4 reserved for Retro Rocket RetroFS partitions
 */
#define RFS_GPT_GUID		"4DEC1156-FEC8-4495-854B-20D888E21AF0"

typedef struct rfs_description_block_t {
	uint64_t identifier; // "RetroFS1"
	uint64_t root_directory;
	uint64_t free_space_map_start;
	uint64_t free_space_map_length;
	uint64_t free_space_map_checksum;
	uint64_t sequence;
	time_t creation_time;
} __attribute__((packed)) rfs_description_block_t;

typedef union rfs_description_block_padded_t {
	rfs_description_block_t desc;
	char raw[RFS_SECTOR_SIZE];
} __attribute__((packed)) rfs_description_block_padded_t;

typedef struct rfs_free_space_map_part_t {
	uint64_t bits[RFS_FS_MAP_BITS_PER_SECTOR];
} __attribute__((packed)) rfs_free_space_map_part_t;

typedef struct rfs_directory_entry_inner_t {
	uint32_t flags;
	char filename[RFS_MAX_NAME];
	uint64_t sector_start;
	uint64_t length;
	time_t created;
	time_t modified;
	uint64_t sequence;
	char reserved[];
} __attribute__((packed)) rfs_directory_entry_inner_t;

typedef struct rfs_directory_start_t {
	uint32_t flags;
	char title[RFS_MAX_NAME];
	uint64_t parent;
	uint64_t sectors;
	uint64_t continuation;
	char reserved[];
} __attribute__((packed)) rfs_directory_start_t;

typedef union rfs_directory_entry_t {
	rfs_directory_start_t start;
	rfs_directory_entry_inner_t entry;
	char raw[RFS_SECTOR_SIZE / 2];
} __attribute__((packed)) rfs_directory_entry_t;

typedef struct rfs_t {
	storage_device_t* dev;
	uint64_t start;
	uint64_t length;
	rfs_description_block_t* desc;
} rfs_t;

void init_rfs();

_Static_assert(sizeof(rfs_directory_entry_t) == (RFS_SECTOR_SIZE / 2), "Directory entry must be exactly half a sector");
_Static_assert(sizeof(rfs_description_block_padded_t) == RFS_SECTOR_SIZE, "Description block must be exactly one sector");
