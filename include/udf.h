/**
 * @file udf.h
 * @brief Structures and helpers for UDF filesystem support.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2026
 */
#pragma once
#include <kernel.h>

/**
 * @brief Logical block size for UDF media
 *
 * UDF optical media is typically fixed at 2048 bytes per block
 */
#define UDF_BLOCK_SIZE 2048

/**
 * @brief Anchor Volume Descriptor Pointer LBA
 *
 * Standard location of the AVDP on UDF media
 */
#define UDF_AVDP_LBA 256

/**
 * @brief Descriptor tag identifiers
 */
typedef enum {
	UDF_TAGID_AVDP = 2,	/**< Anchor Volume Descriptor Pointer */
	UDF_TAGID_PD = 5,	/**< Partition Descriptor */
	UDF_TAGID_LVD = 6,	/**< Logical Volume Descriptor */
	UDF_TAGID_TD = 8,	/**< Terminating Descriptor */
	UDF_TAGID_FSD = 256,	/**< File Set Descriptor */
 	UDF_TAGID_FID = 257,	/**< File Identifier Descriptor */
	UDF_TAGID_FE = 261,	/**< File Entry */
	UDF_TAGID_EFE = 266	/**< Extended file Entry */
} udf_tagid_t;

/* File characteristics flags (FID) */
#define UDF_FILECHAR_HIDDEN    0x01
#define UDF_FILECHAR_DIRECTORY 0x02
#define UDF_FILECHAR_DELETED   0x04
#define UDF_FILECHAR_PARENT    0x08

/**
 * @brief File type value for directories in File Entry.
 */
#define UDF_FT_DIRECTORY 4

/* Allocation descriptor types (ICB flags) */
#define UDF_AD_SHORT 0
#define UDF_AD_LONG  1
#define UDF_AD_INICB 3

/* Extent descriptor flags and masks */
#define UDF_EXTENT_FLAG_MASK   0xc0000000
#define UDF_EXTENT_LENGTH_MASK 0x3fffffff

/**
 * @brief Extent is recorded and allocated (normal data)
 */
#define UDF_EXTENT_RECORDED_ALLOCATED 0

/**
 * @brief Maximum number of partitions tracked per mounted volume
 *
 * This is a fixed upper bound for simplicity
 */
#define MAX_UDF_PARTITIONS 8

/**
 * @brief Maximum directory size considered reasonable
 *
 * Prevents excessive allocation or malformed media causing large reads
 */
#define MAX_REASONABLE_UDF_DIR_SIZE (4 * 1024 * 1024)

/**
 * @brief Maximum number of blocks to read for the Volume Descriptor Sequence
 */
#define MAX_REASONABLE_UDF_VDS_BLOCKS 256

/**
 * @brief Pack a partition number and LBA into a single 64-bit value
 *
 * Stored in fs_directory_entry_t::lbapos
 *
 * @param part Partition number
 * @param lba  Logical block address within the partition
 * @return Packed 64-bit value
 */
#define UDF_PACK_ICB(part, lba) ((((uint64_t)(part)) << 32) | (uint64_t)(lba))

/**
 * @brief Extract partition number from packed ICB value
 *
 * @param v Packed value
 * @return Partition number
 */
#define UDF_ICB_PART(v) ((uint16_t)(((uint64_t)(v) >> 32) & 0xffff))

/**
 * @brief Extract logical block address from packed ICB value
 *
 * @param v Packed value
 * @return Logical block address
 */
#define UDF_ICB_LBA(v) ((uint32_t)((uint64_t)(v) & 0xffffffff))

/**
 * @brief UDF partition descriptor (internal representation)
 *
 * Maps a partition number to its starting LBA on the device
 */
typedef struct udf_partition_t {
	uint16_t number;      /**< Partition number as referenced by descriptors */
	uint32_t start_lba;   /**< Starting LBA of the partition on the device */
} udf_partition_t;

/**
 * @brief Mounted UDF volume state
 *
 * Stores device reference, root directory location, and partition mapping
 * Instances persist for the lifetime of the system
 */
typedef struct udf_t {
	storage_device_t *device; /**< Backing storage device */

	uint16_t root_part;       /**< Partition number of root directory */
	uint32_t root_lba;        /**< Logical block address of root directory */

	char *volume_name;        /**< Volume identifier */

	udf_partition_t partitions[MAX_UDF_PARTITIONS]; /**< Partition table */
	size_t partition_count;                          /**< Number of valid partitions */

	struct udf_t *next; /**< Linked list of mounted UDF volumes */
} udf_t;

/**
 * UDF descriptor tag present at the start of every on-disk descriptor
 *
 * All multi-byte fields are stored in big-endian format
 * The checksum covers the tag itself with the checksum field treated as zero
 */
typedef struct __attribute__((packed)) udf_descriptor_tag_t {
	uint16_t tag_identifier;          /**< Descriptor type (e.g. FE, FID, AVDP, etc) */
	uint16_t descriptor_version;      /**< UDF descriptor version */
	uint8_t tag_checksum;             /**< Checksum of the tag header */
	uint8_t reserved;                 /**< Reserved, must be zero */
	uint16_t tag_serial_number;       /**< Incremented on descriptor rewrite */
	uint16_t descriptor_crc;          /**< CRC of descriptor payload (excluding tag) */
	uint16_t descriptor_crc_length;   /**< Length of data covered by CRC */
	uint32_t tag_location;            /**< Logical block address of this descriptor */
} udf_descriptor_tag_t;

/**
 * UDF ICB (Information Control Block) tag
 *
 * Describes how the file's data and allocation descriptors are organised
 * Embedded within file and directory descriptors
 */
typedef struct __attribute__((packed)) udf_icb_tag_t {
	uint32_t prior_recorded_number_of_direct_entries; /**< Historical entry count (rarely used) */
	uint16_t strategy_type;                           /**< Allocation strategy type */
	uint16_t strategy_parameter;                      /**< Strategy-specific parameter */
	uint16_t maximum_number_of_entries;               /**< Maximum entries for directory-like objects */
	uint8_t reserved;                                 /**< Reserved, must be zero */
	uint8_t file_type;                                /**< File type (directory, regular file, etc) */
	uint32_t parent_icb_location;                     /**< Location of parent ICB (optional usage) */
	uint16_t flags;                                   /**< Allocation descriptor type and flags */
} udf_icb_tag_t;


/**
 * UDF timestamp structure.
 *
 * Represents date/time with sub-second precision and timezone encoding
 * Fields are stored in big-endian format where applicable
 *
 * Timezone is encoded within type_and_timezone
 */
typedef struct __attribute__((packed)) udf_timestamp_t {
	uint16_t type_and_timezone;       /**< Timestamp type and timezone offset */
	uint16_t year;                    /**< Year (e.g. 2025) */
	uint8_t month;                    /**< Month (1–12) */
	uint8_t day;                      /**< Day (1–31) */
	uint8_t hour;                     /**< Hour (0–23) */
	uint8_t minute;                   /**< Minute (0–59) */
	uint8_t second;                   /**< Second (0–59) */
	uint8_t centiseconds;             /**< Hundredths of a second */
	uint8_t hundreds_of_microseconds; /**< 1/10000 second component */
	uint8_t microseconds;             /**< 1/1000000 second component */
} udf_timestamp_t;


/**
 * UDF long allocation descriptor.
 *
 * Describes an extent located within a specific partition.
 * Used when allocation descriptors reference external extents.
 */
typedef struct __attribute__((packed)) udf_long_ad_t {
	uint32_t extent_length;               /**< Length of extent with flag bits in high bits */
	uint32_t extent_location;             /**< Logical block address within partition */
	uint16_t partition_reference_number;  /**< Partition number containing the extent */
	uint8_t implementation_use[6];        /**< Implementation-specific data */
} udf_long_ad_t;


/**
 * UDF file entry descriptor (FE).
 *
 * Describes a file or directory, including metadata, timestamps,
 * and allocation descriptor information.
 *
 * The fixed-size header is followed by:
 *   - Extended attributes (length_of_extended_attributes)
 *   - Allocation descriptors (length_of_allocation_descriptors)
 */
typedef struct __attribute__((packed)) udf_file_entry_t {
	udf_descriptor_tag_t descriptor_tag;  /**< Descriptor tag header */
	udf_icb_tag_t icb_tag;                /**< ICB describing file organisation */

	uint32_t uid;                         /**< Owner user ID */
	uint32_t gid;                         /**< Owner group ID */
	uint32_t permissions;                 /**< File permissions bitmask */
	uint16_t file_link_count;             /**< Number of directory links */

	uint8_t record_format;                /**< Record format (rarely used) */
	uint8_t record_display_attributes;    /**< Display attributes */
	uint32_t record_length;               /**< Record length (for structured files) */

	uint64_t information_length;          /**< Logical file size in bytes */
	uint64_t logical_blocks_recorded;     /**< Blocks actually allocated */

	udf_timestamp_t access_time;          /**< Last access timestamp */
	udf_timestamp_t modification_time;    /**< Last modification timestamp */
	udf_timestamp_t attribute_time;       /**< Last metadata change timestamp */

	uint32_t checkpoint;                  /**< Versioning/checkpoint field */
	udf_long_ad_t extended_attribute_icb; /**< Location of extended attributes (optional) */

	uint32_t implementation_identifier[8]; /**< Implementation identifier (opaque) */
	uint64_t unique_id;                    /**< Unique file identifier */

	uint32_t length_of_extended_attributes;      /**< Size of EA area following header */
	uint32_t length_of_allocation_descriptors;   /**< Size of allocation descriptors area */
} udf_file_entry_t;

/**
 * UDF extended file entry descriptor (EFE).
 *
 * Extended version of the File Entry (FE) used in newer UDF revisions.
 * Replaces FE for a given file when present; both are not used together.
 */
typedef struct __attribute__((packed)) udf_extended_file_entry_t {
	udf_descriptor_tag_t descriptor_tag;  /**< Descriptor tag header */
	udf_icb_tag_t icb_tag;                /**< ICB describing file organisation */

	uint32_t uid;                         /**< Owner user ID */
	uint32_t gid;                         /**< Owner group ID */
	uint32_t permissions;                 /**< File permissions bitmask */
	uint16_t file_link_count;             /**< Number of directory links */

	uint8_t record_format;                /**< Record format (rarely used) */
	uint8_t record_display_attributes;    /**< Display attributes */
	uint32_t record_length;               /**< Record length */

	uint64_t information_length;          /**< Logical file size in bytes */
	uint64_t object_size;                 /**< Physical object size (may differ) */
	uint64_t logical_blocks_recorded;     /**< Blocks actually allocated */

	udf_timestamp_t access_time;          /**< Last access timestamp */
	udf_timestamp_t modification_time;    /**< Last modification timestamp */
	udf_timestamp_t creation_time;        /**< Creation timestamp */
	udf_timestamp_t attribute_time;       /**< Last metadata change timestamp */

	uint32_t checkpoint;                  /**< Versioning/checkpoint field */
	udf_long_ad_t extended_attribute_icb; /**< Location of extended attributes */
	udf_long_ad_t stream_directory_icb;   /**< Stream directory (forks/ADS) */

	uint32_t implementation_identifier[8]; /**< Implementation identifier */
	uint64_t unique_id;                    /**< Unique file identifier */

	uint32_t length_of_extended_attributes;      /**< Size of EA area */
	uint32_t length_of_allocation_descriptors;   /**< Size of allocation descriptors */
} udf_extended_file_entry_t;

/**
 * UDF logical block address.
 *
 * Identifies a logical block within a specific partition.
 */
typedef struct __attribute__((packed)) udf_lb_addr_t {
	uint32_t logical_block_number;       /**< Logical block number within the partition */
	uint16_t partition_reference_number; /**< Partition containing the block */
} udf_lb_addr_t;

/**
 * UDF file identifier descriptor (FID).
 *
 * This is the fixed-size portion of a directory entry record.
 * It is followed by:
 *   - Implementation use area (length_of_implementation_use bytes)
 *   - File identifier bytes (length_of_file_identifier bytes)
 *   - Padding to a 4-byte boundary
 */
typedef struct __attribute__((packed)) udf_file_identifier_descriptor_t {
	udf_descriptor_tag_t descriptor_tag;   /**< Descriptor tag header */
	uint16_t file_version_number;          /**< Version number of the file identifier */
	uint8_t file_characteristics;          /**< File characteristic flags */
	uint8_t length_of_file_identifier;     /**< Length of encoded file identifier */
	uint16_t length_of_implementation_use; /**< Length of implementation use area */
	udf_long_ad_t icb;                     /**< ICB of the referenced file entry */
} udf_file_identifier_descriptor_t;

/**
 * @brief Initialise and register the UDF filesystem driver
 *
 * Allocates and registers the filesystem_t structure and makes the driver
 * available to the VFS
 */
void init_udf();
