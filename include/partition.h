/**
 * @file partition.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 * @brief Definitions and structures for working with MBR and GPT partitions.
 */
#pragma once
#include "kernel.h"

/**
 * @brief Offset of the partition table in the MBR.
 */
#define PARTITION_TABLE_OFFSET 0x1BE

/**
 * @brief ID of a GPT protective partition entry.
 */
#define PARTITION_GPT_PROTECTIVE 0xEE

/**
 * @brief Length of a GUID in ASCII (including dashes, excluding null terminator).
 */
#define GUID_ASCII_LEN 36

/**
 * @brief Length of a GUID in binary form.
 */
#define GUID_BINARY_LEN 16

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
	uint8_t disk_guid[GUID_BINARY_LEN];

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
	uint8_t type_guid[GUID_BINARY_LEN];

	/** @brief Unique GUID for this partition. */
	uint8_t unique_id[GUID_BINARY_LEN];

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
 * @brief Find a partition of a given type on a device.
 *
 * Given a device name and partition type ID, searches for a matching partition
 * and fills the provided output parameters with information about it.
 *
 * @param device_name Name of the device to search.
 * @param partition_type MBR partition type ID to look for.
 * @param found_guid Buffer to store the GUID of the found partition (if GPT).
 * @param partition_type_guid GUID string of the partition type to match (if GPT).
 * @param partition_id Filled with the MBR partition index, or 0xFF if GPT.
 * @param start Filled with the start LBA if found.
 * @param length Filled with the length in sectors if found.
 * @return true if partition found and output parameters are filled.
 * @return false if no matching partition was found.
 */
bool find_partition_of_type(const char* device_name, uint8_t partition_type, char* found_guid, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length);

/**
 * @brief Convert a GUID from ASCII to binary format.
 *
 * @param guid ASCII string representation of the GUID.
 * @param binary Output buffer for binary GUID (16 bytes).
 * @return true if conversion succeeded, false on error.
 */
bool guid_to_binary(const char* guid, void* binary);

/**
 * @brief Convert a binary GUID to ASCII format.
 *
 * @param binary Binary GUID (16 bytes).
 * @param guid Output buffer for ASCII GUID (37 bytes including null terminator).
 * @return true if conversion succeeded, false on error.
 */
bool binary_to_guid(const void* binary, char* guid);
