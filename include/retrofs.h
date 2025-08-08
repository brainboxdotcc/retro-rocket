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

#define RFS_MAP_READ_CHUNK_SECTORS 128ULL

/**
 * @brief Default directory size of a block in sectors
 * (rfs_directory_start_t::sectors)
 * 64 sectors holds 127 files (the first entry is the start
 * entry that contains the continuation pointer etc)
 */
#define RFS_DEFAULT_DIR_SIZE	64

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
	uint64_t total_sectors;
	uint64_t l1_groups;
	uint64_t l2_groups;
	// L1: per-group free counters and bitsets
	uint16_t *l1_free_count;    // [l1_groups], 0..RFS_L1_GROUP_SECTORS
	uint8_t  *l1_not_full;      // bitset, 1 if group has any free sector
	uint8_t  *l1_all_free;      // bitset, 1 if group is entirely free
	// L2: super-group bitsets (over L1)
	uint8_t  *l2_not_full;      // bitset, 1 if any child L1 group has free
	uint8_t  *l2_all_free;      // bitset, 1 if all child L1 groups are fully free

} rfs_t;

// --- free space hierarchy parameters ---
// One L1 group summarises this many sectors (1 bit per sector in L0).
// 4096 sectors == 2 MiB at 512B sectors; tune as you like.
#define RFS_L1_GROUP_SECTORS   (4096ULL)

// One L2 super-group summarises this many L1 groups.
#define RFS_L2_GROUPS_PER_SUPER (1024ULL)

// Bitset helpers
static inline void bitset_set(uint8_t *bs, uint64_t idx, bool val) {
	uint64_t byte = idx >> 3;
	uint8_t  mask = (uint8_t)(1u << (idx & 7));
	if (val) bs[byte] |= mask; else bs[byte] &= (uint8_t)~mask;
}

static inline bool bitset_get(const uint8_t *bs, uint64_t idx) {
	uint64_t byte = idx >> 3;
	uint8_t  mask = (uint8_t)(1u << (idx & 7));
	return (bs[byte] & mask) != 0;
}

static inline size_t bitset_bytes(uint64_t nbits) {
	return (size_t)((nbits + 7ULL) >> 3);
}


void init_rfs();
bool rfs_format(rfs_t* info);
int rfs_read_device(rfs_t* rfs, uint64_t start_sectors, uint64_t size_bytes, void* buffer);
int rfs_write_device(rfs_t* rfs, uint64_t start_sectors, uint64_t size_bytes, const void* buffer);
bool rfs_find_free_extent(rfs_t *info, uint64_t need, uint64_t *out_start_sector);
bool rfs_mark_extent(rfs_t *info, uint64_t start_sector, uint64_t length_sectors, bool mark_used);
bool rfs_build_level_caches(rfs_t *info);

void* rfs_get_directory(void* t);
bool rfs_unlink_file(void* dir, const char* name);
bool rfs_read_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer);
uint64_t rfs_create_file(void* dir, const char* name, size_t size);
uint64_t rfs_create_directory(void* dir, const char* name);
bool rfs_unlink_dir(void* dir, const char* name);
bool rfs_write_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer);
bool rfs_truncate_file(void* f, size_t length);

_Static_assert(sizeof(rfs_directory_entry_t) == (RFS_SECTOR_SIZE / 2), "Directory entry must be exactly half a sector");
_Static_assert(sizeof(rfs_description_block_padded_t) == RFS_SECTOR_SIZE, "Description block must be exactly one sector");
_Static_assert(RFS_MAP_READ_CHUNK_SECTORS * RFS_SECTOR_SIZE <= (4ULL * 1024 * 1024), "AHCI PRDT entry must be <= 4 MiB");