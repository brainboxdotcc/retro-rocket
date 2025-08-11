/**
 * @file retrofs.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Core structures, constants, and API definitions for RetroFS — an
 *        ADFS 'L' format–inspired filesystem for Retro Rocket.
 *
 * RetroFS is a sector-based filesystem inspired by the Acorn ADFS 'L' format.
 * It is optimised for simplicity of implementation and fast free-space
 * allocation, achieved by caching the entire multi-level free space map
 * in RAM. This approach trades higher memory usage (~200 MB/TB) for minimal
 * device I/O during allocation and deletion, making it well-suited to systems
 * where RAM is plentiful relative to storage speed.
 *
 * The on-disk format is compact and self-contained, with a fixed-size
 * description block, hierarchical free space map, and half-sector
 * directory entries. The in-memory layout is designed for fast lookups
 * and minimal pointer chasing.
 *
 * @copyright (c) 2012–2025 Craig Edwards
 */

#pragma once

#include <kernel.h>

/** @name Filesystem Constants
 *  @{
 */
#define RFS_SECTOR_SIZE 512             /**< Size of one logical sector in bytes. */
#define RFS_MAX_NAME 128                 /**< Maximum length of file/directory name (excluding terminator). */
#define RFS_FS_MAP_BITS_PER_SECTOR (512 / sizeof(uint64_t)) /**< Number of free-space bits stored in a sector. */

#define RFS_MAP_READ_CHUNK_SECTORS 16 /**< Preferred chunk size (in sectors) for reading the free space map. */

/**
 * @brief Default number of sectors allocated for a directory block.
 *
 * The first entry is a directory start entry containing metadata and a
 * continuation pointer; remaining entries store files.
 * At 64 sectors, a directory can contain up to 127 files.
 */
#define RFS_DEFAULT_DIR_SIZE 64

/* Directory entry flags */
#define RFS_FLAG_DIRECTORY 0x0001   /**< Entry is a directory. */
#define RFS_FLAG_LOCKED    0x0002   /**< Entry is locked/read-only. */
#define RFS_FLAG_DIR_START 0x0004   /**< Entry is a directory start block. */

/**
 * @brief Filesystem identifier magic ("RetroFS1").
 *
 * Stored in the description block for on-disk format verification.
 */
#define RFS_ID (uint64_t)0x3153466f72746552ULL

/**
 * @brief RetroFS GPT Partition Type GUID.
 *
 * This UUID v4 is reserved for Retro Rocket's RetroFS partitions.
 */
#define RFS_GPT_GUID "4DEC1156-FEC8-4495-854B-20D888E21AF0"
/** @} */

/**
 * @brief On-disk description block containing global filesystem metadata.
 *
 * Occupies exactly one sector. This is the first structure read when mounting
 * a RetroFS volume.
 */
typedef struct rfs_description_block_t {
	uint64_t identifier;              /**< Filesystem magic (RFS_ID). */
	uint64_t root_directory;           /**< Sector address of root directory start entry. */
	uint64_t free_space_map_start;     /**< Sector address of free space map start. */
	uint64_t free_space_map_length;    /**< Length of free space map in sectors. */
	uint64_t free_space_map_checksum;  /**< Checksum of the free space map. */
	uint64_t sequence;                 /**< Incremented whenever the FS is modified. */
	time_t creation_time;            /**< Filesystem creation time (UTC). */
} __attribute__((packed)) rfs_description_block_t;

/**
 * @brief Padded variant of the description block to exactly fill one sector.
 */
typedef union rfs_description_block_padded_t {
	rfs_description_block_t desc;      /**< Structured view. */
	char raw[RFS_SECTOR_SIZE];         /**< Raw bytes view. */
} __attribute__((packed)) rfs_description_block_padded_t;

/**
 * @brief One sector of the free space map, containing an array of 64-bit bitfields.
 */
typedef struct rfs_free_space_map_part_t {
	uint64_t bits[RFS_FS_MAP_BITS_PER_SECTOR]; /**< Free/used sector flags. */
} __attribute__((packed)) rfs_free_space_map_part_t;

/**
 * @brief On-disk file entry structure for non-directory entries.
 */
typedef struct rfs_directory_entry_inner_t {
	uint32_t flags;                 /**< Entry flags (RFS_FLAG_*). */
	char filename[RFS_MAX_NAME];    /**< Null-terminated filename. */
	uint64_t sector_start;          /**< First sector of file data. */
	uint64_t length;                /**< File length in bytes. */
	uint64_t sector_length;         /**< Allocated length in sectors */
	time_t created;                 /**< Creation timestamp (UTC). */
	time_t modified;                /**< Last modification timestamp (UTC). */
	uint64_t sequence;              /**< Incremented when file changes. */
	char reserved[];                /**< Reserved for future use / alignment. */
} __attribute__((packed)) rfs_directory_entry_inner_t;

/**
 * @brief On-disk directory start entry.
 *
 * This is always the first entry in a directory block.
 */
typedef struct rfs_directory_start_t {
	uint32_t flags;                     /**< Entry flags (must include RFS_FLAG_DIR_START). */
	char title[RFS_MAX_NAME];        /**< Human-readable directory title. */
	uint64_t parent;                     /**< Sector of parent directory start entry. */
	uint64_t sectors;                    /**< Number of sectors in this directory block. */
	uint64_t continuation;               /**< Sector of next directory block, or 0 if none. */
	char reserved[];                  /**< Reserved for future use / alignment. */
} __attribute__((packed)) rfs_directory_start_t;

/**
 * @brief Unified representation of either a directory start or file entry.
 *
 * Each directory block sector contains two of these (half-sector entries).
 */
typedef union rfs_directory_entry_t {
	rfs_directory_start_t start;  /**< Directory start entry. */
	rfs_directory_entry_inner_t entry;  /**< File entry. */
	char raw[RFS_SECTOR_SIZE / 2]; /**< Raw bytes view. */
} __attribute__((packed)) rfs_directory_entry_t;

/**
 * @brief In-memory RetroFS mount context.
 *
 * Holds the filesystem’s geometry, cached metadata, and free space tracking structures.
 */
typedef struct rfs_t {
	storage_device_t *dev;               /**< Backing block device. */
	uint64_t start;                      /**< Start sector of the filesystem on the device. */
	uint64_t length;                     /**< Length of the filesystem in sectors. */
	rfs_description_block_t *desc;       /**< Pointer to loaded description block. */
	uint64_t total_sectors;              /**< Total sectors in this filesystem. */

	void *cache_block;                  /**< Owning pointer for all L1/L2 arrays and build buffer. */
	size_t cache_block_size;             /**< Allocated cache block size (for debugging). */

	uint64_t l1_groups;                   /**< Number of L1 groups. */
	uint64_t l2_groups;                   /**< Number of L2 super-groups. */

	/* L1: per-group free counters and bitsets */
	uint16_t *l1_free_count;               /**< Array of free sector counts for each L1 group. */
	uint8_t *l1_not_full;                  /**< Bitset: group has any free sector. */
	uint8_t *l1_all_free;                  /**< Bitset: group is entirely free. */

	/* L2: super-group bitsets summarising L1 */
	uint8_t *l2_not_full;                  /**< Bitset: super-group has any free sector. */
	uint8_t *l2_all_free;                  /**< Bitset: super-group is entirely free. */
} rfs_t;

#define RFS_L1_GROUP_SECTORS    (4096ULL) /**< Sectors per L1 group (2 MiB at 512B). */
#define RFS_L2_GROUPS_PER_SUPER (1024ULL) /**< L1 groups per L2 super-group. */

/**
 * @brief Get the default reservation size for a file based on extension.
 *
 * @param filename Filename to check.
 * @return Size in bytes to reserve. Defaults to 128 KB if not in table.
 */
size_t rfs_get_default_reservation(const char* filename);

/**
 * @brief Set or clear a bit in a bitset.
 * @param bs Pointer to bitset.
 * @param idx Bit index.
 * @param val New value (true = set, false = clear).
 */
static inline void bitset_set(uint8_t *bs, uint64_t idx, bool val) {
	uint64_t byte = idx >> 3;
	uint8_t mask = (uint8_t) (1u << (idx & 7));
	if (val) {
		bs[byte] |= mask;
	} else {
		bs[byte] &= (uint8_t) ~mask;
	}
}

/**
 * @brief Get the value of a bit from a bitset.
 * @param bs Pointer to bitset.
 * @param idx Bit index.
 * @return True if bit is set, false if clear.
 */
static inline bool bitset_get(const uint8_t *bs, uint64_t idx) {
	uint64_t byte = idx >> 3;
	uint8_t mask = (uint8_t) (1u << (idx & 7));
	return (bs[byte] & mask) != 0;
}

/**
 * @brief Calculate the number of bytes needed to store a bitset of given size.
 * @param nbits Number of bits.
 * @return Number of bytes required.
 */
static inline size_t bitset_bytes(uint64_t nbits) {
	return (size_t) ((nbits + 7ULL) >> 3);
}

/** @brief Align a value upwards to the next multiple of `a`. */
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

/**
 * @brief Initialise the RetroFS subsystem.
 *
 * Prepares any global state needed for RetroFS operation.
 * This should be called once during kernel initialisation before
 * mounting any RetroFS volumes.
 */
void init_rfs(void);

/**
 * @brief Format a block device or partition with a fresh RetroFS filesystem.
 *
 * Writes a new description block, initialises the free space map,
 * and creates an empty root directory.
 *
 * @param info Pointer to an rfs_t structure describing the target volume.
 * @return true on success, false on error (e.g., device write failure).
 */
bool rfs_format(rfs_t *info);

/**
 * @brief Read raw sectors from the underlying device.
 *
 * Low-level helper for retrieving a contiguous range of sectors
 * from the block device.
 *
 * @param rfs           Filesystem context.
 * @param start_sectors Sector index (relative to filesystem start).
 * @param size_bytes    Number of bytes to read (must be a multiple of sector size).
 * @param buffer        Destination buffer.
 * @return true on success, false on error.
 */
bool rfs_read_device(rfs_t *rfs, uint64_t start_sectors, uint64_t size_bytes, void *buffer);

/**
 * @brief Write raw sectors to the underlying device.
 *
 * Low-level helper for storing a contiguous range of sectors
 * to the block device.
 *
 * @param rfs           Filesystem context.
 * @param start_sectors Sector index (relative to filesystem start).
 * @param size_bytes    Number of bytes to write (must be a multiple of sector size).
 * @param buffer        Source buffer.
 * @return true on success, false on error.
 */
bool rfs_write_device(rfs_t *rfs, uint64_t start_sectors, uint64_t size_bytes, const void *buffer);

/**
 * @brief Find a contiguous extent of free sectors.
 *
 * Searches the in-memory free space map for an extent of at least
 * the requested length.
 *
 * @param info             Filesystem context.
 * @param need             Number of sectors required.
 * @param out_start_sector Output: first sector of found extent.
 * @return true if an extent was found, false if insufficient free space.
 */
bool rfs_find_free_extent(rfs_t *info, uint64_t need, uint64_t *out_start_sector);

/**
 * @brief Mark a contiguous extent as used or free in the free space map.
 *
 * Updates the in-memory free space tracking structures; caller must
 * ensure changes are persisted to disk if necessary.
 *
 * @param info          Filesystem context.
 * @param start_sector  First sector of extent.
 * @param length_sectors Number of sectors in extent.
 * @param mark_used     true to mark as used, false to mark as free.
 * @return true on success, false on error (e.g., bounds check fail).
 */
bool rfs_mark_extent(rfs_t *info, uint64_t start_sector, uint64_t length_sectors, bool mark_used);

/**
 * @brief Build L1/L2 cache structures from the on-disk free space map.
 *
 * Reads the free space map from disk and generates the in-memory
 * summary structures used for fast allocation.
 *
 * @param info Filesystem context.
 * @return true on success, false on error.
 */
bool rfs_build_level_caches(rfs_t *info);

/**
 * @brief Retrieve a directory structure from a VFS handle.
 *
 * Used by the VFS layer to resolve a directory handle into a
 * RetroFS directory block for further operations.
 *
 * @param t VFS handle for a directory.
 * @return Pointer to RetroFS directory block, or NULL on error.
 */
void *rfs_get_directory(void *t);

/**
 * @brief Remove a file from a directory.
 *
 * Deletes the directory entry and frees the associated file data
 * sectors in the free space map.
 *
 * @param dir  Pointer to directory block.
 * @param name Filename to remove.
 * @return true on success, false if not found or error occurred.
 */
bool rfs_unlink_file(void *dir, const char *name);

/**
 * @brief Read part or all of a file.
 *
 * Reads data starting at the given offset (in bytes from start of file).
 *
 * @param f       Pointer to file entry.
 * @param start   Offset into file to begin reading.
 * @param length  Number of bytes to read.
 * @param buffer  Destination buffer.
 * @return true on success, false on error or bounds violation.
 */
bool rfs_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer);

/**
 * @brief Create a new file in a directory.
 *
 * Allocates space, writes a directory entry, and sets initial file size.
 *
 * @param dir  Pointer to directory block.
 * @param name Name of new file.
 * @param size Initial size in bytes.
 * @return Sector number of new file’s start, or 0 on failure.
 */
uint64_t rfs_create_file(void *dir, const char *name, size_t size);

/**
 * @brief Create a new directory within an existing directory.
 *
 * Allocates space for the directory start block and updates the parent.
 *
 * @param dir  Pointer to parent directory block.
 * @param name Name of new directory.
 * @return Sector number of new directory’s start block, or 0 on failure.
 */
uint64_t rfs_create_directory(void *dir, const char *name);

/**
 * @brief Remove an empty directory from its parent.
 *
 * @param dir  Pointer to parent directory block.
 * @param name Name of directory to remove.
 * @return true on success, false if directory is not empty or not found.
 */
bool rfs_unlink_dir(void *dir, const char *name);

/**
 * @brief Write part or all of a file.
 *
 * Writes data starting at the given offset (in bytes from start of file).
 * Caller must ensure enough space has been allocated beforehand.
 *
 * @param f       Pointer to file entry.
 * @param start   Offset into file to begin writing.
 * @param length  Number of bytes to write.
 * @param buffer  Source buffer.
 * @return true on success, false on error or bounds violation.
 */
bool rfs_write_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer);

/**
 * @brief Truncate or extend a file to a given length.
 *
 * If truncating, deallocates excess sectors.
 * If extending, allocates new sectors (contents unspecified).
 *
 * @param f      Pointer to file entry.
 * @param length New size in bytes.
 * @return true on success, false on error or insufficient space.
 */
bool rfs_truncate_file(void *f, size_t length);

/**
 * @brief Insert or update a directory entry on disk.
 *
 * If an entry with the same name (case-insensitive) already exists in the
 * directory chain, its metadata is updated in-place, preserving the case
 * of the supplied @p file->filename. If no matching entry exists, a new one
 * is appended in the first available slot; if the directory is full, a new
 * continuation block is allocated and linked.
 *
 * This function does not allocate or free file payload sectors if the file
 * size changes; it is limited to updating directory metadata and allocating
 * directory blocks when necessary.
 *
 * @pre  @p file must be non-NULL.
 * @pre  @p file->directory must point to a valid fs_tree_t representing the
 *       parent directory.
 * @pre  @p file->filename must point to a non-empty NUL-terminated string.
 * @post On success, the directory chain on disk contains the inserted or
 *       updated entry, and @p file reflects the same metadata supplied.
 *
 * @param file Pointer to a VFS directory entry to insert or update.
 * @param sector_extent Reserved extent for the file in sectors
 * @return The same @p file pointer on success, or NULL on error.
 */
fs_directory_entry_t* rfs_upsert_directory_entry(fs_directory_entry_t* file, uint64_t sector_extent);

/**
 * @brief Remove a directory entry from disk by name.
 *
 * Finds the first entry in the directory chain whose name matches
 * @p file->filename case-insensitively, removes it, and compacts the
 * remaining entries in that block to avoid internal holes. Continuation
 * blocks are not freed, even if they become empty.
 *
 * This function does not modify the free-space map for the file payload;
 * only the directory metadata is altered.
 *
 * @pre  @p file must be non-NULL.
 * @pre  @p file->directory must point to a valid fs_tree_t representing the
 *       parent directory.
 * @pre  @p file->filename must point to a non-empty NUL-terminated string.
 * @post On success, the directory chain on disk no longer contains an entry
 *       matching @p file->filename in a case-insensitive comparison.
 *
 * @param file Pointer to a VFS directory entry identifying the file to remove.
 * @return true on success, false if the entry was not found or on I/O error.
 */
bool rfs_delete_directory_entry(fs_directory_entry_t* file);

/**
 * @brief Convert a size in bytes to the number of sectors required.
 *
 * This rounds up to the nearest whole sector, so that any non-zero
 * remainder results in an additional sector being counted.
 *
 * @param bytes Size in bytes.
 * @return Number of sectors required to store @p bytes.
 */
uint64_t rfs_bytes_to_sectors(uint64_t bytes);

/**
 * @brief Locate a directory entry by name within a given directory.
 *
 * Searches the specified directory (including any continuation blocks)
 * for an entry whose filename matches @p name. If found, returns its
 * on-disk location and a copy of the directory entry structure.
 *
 * @param info            Filesystem context (RetroFS instance).
 * @param tree            VFS tree node representing the directory to search.
 * @param name            Filename to look up (null-terminated string).
 * @param out_sector      Receives the sector number containing the entry.
 * @param out_index       Receives the index of the entry within that sector.
 * @param out_entry_copy  Receives a copy of the on-disk entry.
 * @return @c true if the entry was found, @c false otherwise.
 */
bool rfs_locate_entry(rfs_t* info, fs_tree_t* tree, const char* name, uint64_t* out_sector, size_t* out_index, rfs_directory_entry_inner_t* out_entry_copy);

/**
 * @brief Calculate the exact number of bytes currently in use on the volume.
 *
 * This function uses the in-memory Level-1 free‐sector counters built by
 * rfs_build_level_caches() to determine usage precisely. It sums the free
 * sector counts for all L1 groups, converts the total to bytes, and subtracts
 * from the total volume size. The result is exact as long as the L1 cache is
 * up to date.
 *
 * @param info Pointer to the RetroFS volume context (must have valid L1 cache).
 * @return Number of bytes currently allocated/used on the volume.
 *
 * @note If the volume was not cleanly unmounted, ensure the L1 cache has been
 *       rebuilt from the on-disk free space map before calling this function.
 */
uint64_t rfs_get_used_bytes(rfs_t *info);

/**
 * @brief VFS endpoint to get free space on the filesystem in bytes
 * @param fs fs_tree_t
 * @return free space in bytes
 */
uint64_t rfs_get_free_space(void* fs);

_Static_assert(sizeof(rfs_directory_entry_t) == (RFS_SECTOR_SIZE / 2), "Directory entry must be exactly half a sector");
_Static_assert(sizeof(rfs_description_block_padded_t) == RFS_SECTOR_SIZE, "Description block must be exactly one sector");
_Static_assert(RFS_MAP_READ_CHUNK_SECTORS * RFS_SECTOR_SIZE <= (4ULL * 1024 * 1024), "AHCI PRDT entry must be <= 4 MiB");
