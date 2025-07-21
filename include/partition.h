/**
 * @file partition.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#include "kernel.h"

// Offset of partition table in MBR
#define PARTITION_TABLE_OFFSET	0x1BE

// ID of a GPT protective partition entry
#define PARTITON_GPT_PROTECTIVE 0xEE

// Length of a GUID in ASCII including -, without null terminator
#define GUID_ASCII_LEN	36

// Length of a GUID in binary
#define GUID_BINARY_LEN 16



/**
 * @brief A disk partition on a storage device
 */
typedef struct partition_t {
	/// @brief Partition is bootable
	uint8_t bootable;
	/// @brief Starting head
	uint8_t starthead;
	/// @brief Starting cylinder and sector
	uint16_t startcylsect;
	/// @brief partition type id
	uint8_t systemid;
	/// @brief End head
	uint8_t endhead;
	/// @brief End cylinder and sector
	uint16_t endcylsect;
	/// @brief Starting LBA
	uint32_t startlba;
	/// @brief LBA length
	uint32_t length;
} __attribute__((packed)) partition_t;

/**
 * @brief A disk partition table consisiting of four partition_t
 */
typedef struct partition_table_t {
	/// @brief Partition entries
	partition_t p_entry[4];
} __attribute__((packed)) partition_table_t;

typedef struct gpt_header_t {
	char signature[8];
	uint32_t gpt_revision;
	uint32_t header_size;
	uint32_t crc_checksum;
	uint32_t reserved0;
	uint64_t lba_of_this_header;
	uint64_t lba_of_alternative_header;
	uint64_t first_usable_block;
	uint64_t last_usable_block;
	uint8_t disk_guid[GUID_BINARY_LEN];
	uint64_t lba_of_partition_entries;
	uint32_t number_partition_entries;
	uint32_t size_of_each_entry;
	uint32_t crc_32_of_entries;
	uint8_t reserved1[];
} __attribute__((packed)) gpt_header_t;

typedef struct gpt_entry_t {
	uint8_t type_guid[GUID_BINARY_LEN];
	uint8_t unique_id[GUID_BINARY_LEN];
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attributes;
	char name[72];
} __attribute__((packed)) gpt_entry_t;

/**
 * @brief Given a device name and partition type ID, find the start and length of that partition
 * on the device. Returns false if the partition could not be found.
 * 
 * @param device_name Device name
 * @param partition_type Partition type ID to find
 * @param found_guid filled with the guid of the found partition, if found on a GPT partition
 * @param partition_id Partition ID of found partition, filled if found, or 0xFF if found on a GPT partition
 * @param start Start of found partition, filled if found
 * @param length Length of found partition, filled if found
 * @return true if partition found, partition_id, start and length will be set
 * @return false if partition not found, partition_id, start and length will be unchanged
 */
bool find_partition_of_type(const char* device_name, uint8_t partition_type, char* found_guid, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length);

bool guid_to_binary(const char* guid, void* binary);

bool binary_to_guid(const void* binary, char* guid);