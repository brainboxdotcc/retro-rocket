#pragma once
#include <kernel.h>

/**
 * @brief DFS sector size in bytes
 */
#define DFS_SECTOR_SIZE 256

/**
 * @brief Number of sectors per track on standard DFS discs
 */
#define DFS_SECTORS_PER_TRACK 10

/**
 * @brief Maximum number of catalogue entries per side
 */
#define DFS_MAX_FILES 31

/**
 * @brief Total sectors for a standard single-sided DFS image (.ssd)
 */
#define DFS_IMAGE_SSD_SECTORS 800

/**
 * @brief Total sectors for a standard double-sided DFS image (.dsd)
 */
#define DFS_IMAGE_DSD_SECTORS 1600

/**
 * @brief Flag used in lbapos to indicate a file resides on side 2
 */
#define DFS_LBAPOS_SIDE_FLAG 0x80000000

/**
 * @brief Synthetic lbapos value representing the side1 directory in DSD images
 */
#define DFS_LBAPOS_SIDE1_DIR 0xfffffffe

/**
 * @brief Synthetic lbapos value representing the side2 directory in DSD images
 */
#define DFS_LBAPOS_SIDE2_DIR 0xfffffffd

/**
 * @brief DFS catalogue entry (sector 0)
 *
 * Contains the filename and directory character.
 */
typedef struct __attribute__((packed)) dfs_catalogue_entry {
	char name[7];          /**< Filename (7 chars, space padded, high bit ignored) */
	uint8_t directory;     /**< Directory character (high bit ignored) */
} dfs_catalogue_entry_t;

/**
 * @brief DFS catalogue metadata entry (sector 1)
 *
 * Stores file addresses, length, and start sector using packed fields.
 * Load and execution addresses are present but not used by the driver.
 */
typedef struct __attribute__((packed)) dfs_catalogue_meta {
	uint8_t load_lo;       /**< Load address low byte */
	uint8_t load_mid;      /**< Load address mid byte */
	uint8_t exec_lo;       /**< Exec address low byte */
	uint8_t exec_mid;      /**< Exec address mid byte */
	uint8_t length_lo;     /**< File length low byte */
	uint8_t length_mid;    /**< File length mid byte */
	unsigned int start_hi : 2;   /**< Start sector high bits */
	unsigned int load_hi : 2;    /**< Load address high bits */
	unsigned int exec_hi : 2;    /**< Exec address high bits */
	unsigned int length_hi : 2;  /**< File length high bits */
	uint8_t start_lo;      /**< Start sector low byte */
} dfs_catalogue_meta_t;

/**
 * @brief DFS catalogue sector 0
 *
 * Contains disc title and file entries.
 */
typedef struct __attribute__((packed)) dfs_catalogue_sector0 {
	char title[8];                              /**< Disc title (first part) */
	dfs_catalogue_entry_t entries[DFS_MAX_FILES]; /**< File entries */
} dfs_catalogue_sector0_t;

/**
 * @brief DFS catalogue sector 1
 *
 * Contains additional title data, entry count, sector count, and metadata.
 */
typedef struct __attribute__((packed)) dfs_catalogue_sector1 {
	char title_tail[4];                         /**< Disc title continuation */
	uint8_t cycle;                              /**< Catalogue sequence number */
	uint8_t entry_count_x8;                     /**< Entry count multiplied by 8 */
	uint8_t sector_hi;                          /**< Sector count high bits + boot option */
	uint8_t sector_lo;                          /**< Sector count low byte */
	dfs_catalogue_meta_t meta[DFS_MAX_FILES];   /**< File metadata entries */
} dfs_catalogue_sector1_t;

/**
 * @brief DFS filesystem instance
 *
 * Represents a mounted DFS volume, either SSD or DSD.
 */
typedef struct dfs {
	storage_device_t *device;  /**< Backing storage device */
	bool is_dsd;               /**< True if image is double-sided */
	uint32_t sectors_per_side; /**< Number of sectors per side */
	bool side_valid[2];        /**< Per-side validity flags (for DSD) */
} dfs_t;

/**
 * @brief Initialise the DFS filesystem driver
 *
 * Registers the DFS filesystem with the VFS.
 */
void init_dfs(void);

/**
 * @brief Initialise a ramdisk from a DFS image
 *
 * @param image Pointer to raw DFS image data
 * @param length Length of the image in bytes
 * @return Device name on success, NULL on failure
 */
const char *init_ramdisk_from_dfs_image(uint8_t *image, size_t length);