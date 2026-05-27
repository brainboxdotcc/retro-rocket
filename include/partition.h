/**
 * @file partition.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 * @brief Definitions and structures for working with MBR and GPT partitions.
 */
#pragma once
#include "kernel.h"
#include "guid.h"

/**
 * @brief Offset of the partition table in the MBR.
 */
#define PARTITION_TABLE_OFFSET 0x1BE

/**
 * @brief ID of a GPT protective partition entry.
 */
#define PARTITION_GPT_PROTECTIVE 0xEE

/**
 * @brief Pass this to filesytem_mount to scan for partitions instead of specifying
 */
#define PARTITION_FIRST_MATCH (int)-1

/**
 * @brief MBR partition type used for Linux LVM physical volumes.
 */
#define PARTITION_LVM 0x8E

/**
 * @brief GPT partition type GUID used for Linux LVM physical volumes.
 */
#define GPT_LINUX_LVM "E6D6D379-F507-44C2-A23C-238F2A3DF928"

/**
 * @brief Sector containing the LVM label relative to the start of the physical volume.
 */
#define LVM_LABEL_SECTOR 1

/**
 * @brief Maximum LVM metadata area size accepted by the lightweight parser.
 */
#define LVM_MAX_METADATA_SIZE (1024 * 1024)

/**
 * @brief A disk partition entry within the MBR.
 */
typedef struct partition_t {
	/** @brief Partition is bootable (0x80 = bootable, 0x00 = non-bootable). */
	uint8_t bootable;

	/** @brief Starting head of the partition. */
	uint8_t starthead;

	/** @brief Starting cylinder and sector of the partition. */
	uint16_t startcylsect;

	/** @brief Partition type identifier. */
	uint8_t systemid;

	/** @brief Ending head of the partition. */
	uint8_t endhead;

	/** @brief Ending cylinder and sector of the partition. */
	uint16_t endcylsect;

	/** @brief Starting LBA (Logical Block Address) of the partition. */
	uint32_t startlba;

	/** @brief Length of the partition in sectors. */
	uint32_t length;
} __attribute__((packed)) partition_t;

/**
 * @brief A disk partition table consisting of four MBR partition entries.
 */
typedef struct partition_table_t {
	/** @brief Array of four partition entries. */
	partition_t p_entry[4];
} __attribute__((packed)) partition_table_t;

/**
 * @brief The GPT (GUID Partition Table) header.
 */
typedef struct gpt_header_t {
	/** @brief GPT signature, should be "EFI PART". */
	char signature[8];

	/** @brief GPT revision number. */
	uint32_t gpt_revision;

	/** @brief Size of the GPT header in bytes. */
	uint32_t header_size;

	/** @brief CRC32 checksum of the GPT header. */
	uint32_t crc_checksum;

	/** @brief Reserved field, must be zero. */
	uint32_t reserved0;

	/** @brief LBA of this GPT header. */
	uint64_t lba_of_this_header;

	/** @brief LBA of the alternative (backup) GPT header. */
	uint64_t lba_of_alternative_header;

	/** @brief First usable LBA for partitions. */
	uint64_t first_usable_block;

	/** @brief Last usable LBA for partitions. */
	uint64_t last_usable_block;

	/** @brief Disk GUID in binary form. */
	binary_guid_t disk_guid;

	/** @brief Starting LBA of the partition entries array. */
	uint64_t lba_of_partition_entries;

	/** @brief Number of partition entries. */
	uint32_t number_partition_entries;

	/** @brief Size of each partition entry in bytes. */
	uint32_t size_of_each_entry;

	/** @brief CRC32 checksum of the partition entries array. */
	uint32_t crc_32_of_entries;

	/** @brief Reserved space, must be zero. */
	uint8_t reserved1[];
} __attribute__((packed)) gpt_header_t;

/**
 * @brief A single GPT partition entry.
 */
typedef struct gpt_entry_t {
	/** @brief Partition type GUID. */
	binary_guid_t type_guid;

	/** @brief Unique GUID for this partition. */
	binary_guid_t unique_id;

	/** @brief Starting LBA of the partition. */
	uint64_t start_lba;

	/** @brief Ending LBA of the partition. */
	uint64_t end_lba;

	/** @brief Partition attributes (bit flags). */
	uint64_t attributes;

	/** @brief Partition name (UTF-16LE, up to 36 characters). */
	char name[72];
} __attribute__((packed)) gpt_entry_t;

/**
 * @brief LVM label header stored at the label sector of a physical volume.
 */
typedef struct lvm_label_header {
	/** @brief Label identifier, normally "LABELONE". */
	char id[8];

	/** @brief Sector number where this label was found. */
	uint64_t sector;

	/** @brief CRC of the label header. */
	uint32_t crc;

	/** @brief Offset from the start of the sector to the LVM physical volume header. */
	uint32_t offset;

	/** @brief Label type, normally "LVM2 001". */
	char type[8];
} __attribute__((packed)) lvm_label_header_t;

/**
 * @brief Offset and size pair used by LVM physical volume metadata structures.
 */
typedef struct lvm_disk_location {
	/** @brief Byte offset relative to the start of the physical volume. */
	uint64_t offset;

	/** @brief Size in bytes. */
	uint64_t size;
} __attribute__((packed)) lvm_disk_location_t;

/**
 * @brief LVM physical volume header referenced by the label header.
 */
typedef struct lvm_pv_header {
	/** @brief LVM physical volume UUID as stored on disk. */
	char uuid[32];

	/** @brief Size of the physical volume device in bytes. */
	uint64_t device_size;

	/** @brief Location of the physical extent data area. */
	lvm_disk_location_t data_area;

	/** @brief Location of the first metadata area. */
	lvm_disk_location_t metadata_area;
} __attribute__((packed)) lvm_pv_header_t;

/**
 * @brief Callback invoked while enumerating visible volumes.
 *
 * @param index Visible flattened volume index.
 * @param matched true if this volume matches the requested search criteria.
 * @param description Human-readable description of the volume.
 * @param opaque user data
 */
typedef void (*enumerator_fn_t)(int8_t index, bool matched, const char* description, void* opaque);

typedef struct volume_enumerator_t {
	enumerator_fn_t fn;
	void* opaque;
} volume_enumerator_t;

/**
 * @brief Find a visible volume of a requested partition type on a device.
 *
 * Searches the device for volumes matching the requested MBR partition type or
 * GPT partition type GUID. On MBR disks, normal partition entries are matched
 * against @p partition_type. On GPT disks, partition entries are matched against
 * @p partition_type_guid.
 *
 * Linux LVM physical volumes are also scanned. Simple linear LVM logical volumes
 * are folded into the same visible index sequence as ordinary partitions, but
 * are only returned when the requested type represents a Linux filesystem
 * partition. This keeps volume numbering stable while avoiding exposing LVM
 * contents for unrelated partition searches.
 *
 * @param device_name Name of the device to search.
 * @param partition_type MBR partition type ID to look for.
 * @param found_guid Buffer to store the unique GPT partition GUID if a GPT partition is found.
 * @param partition_type_guid GPT partition type GUID string to look for.
 * @param partition_id Filled with the MBR partition index, 0xFF for GPT, or 0xFE for LVM.
 * @param start Filled with the start LBA of the matched volume.
 * @param length Filled with the length of the matched volume in sectors.
 * @param start_index First visible matching index to consider, inclusive.
 * @param end_index Last visible matching index to consider, inclusive.
 * @param walk If non-NULL, will be called for each item the function iterates over including any match
 * @return true if a matching volume was found and output parameters were filled.
 * @return false if no matching volume was found.
 */
bool find_partition_of_type(const char* device_name, uint8_t partition_type, text_guid_t found_guid, const text_guid_t partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length, uint8_t start_index, uint8_t end_index, volume_enumerator_t* walk);
