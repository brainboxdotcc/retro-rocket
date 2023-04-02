#pragma once
#include "kernel.h"

// Offset of partition table in MBR
#define PARTITION_TABLE_OFFSET	0x1BE

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

/**
 * @brief Given a device name and partition type ID, find the start and length of that partition
 * on the device. Returns false if the partition could not be found.
 * 
 * @param device_name Device name
 * @param partition_type Partition type ID to find
 * @param partition_id Partition ID of found partition, filled if found
 * @param start Start of found partition, filled if found
 * @param length Length of found partition, filled if found
 * @return true if partition found, partition_id, start and length will be set
 * @return false if partition not found, partition_id, start and length will be unchanged
 */
bool find_partition_of_type(const char* device_name, uint8_t partition_type, uint8_t* partition_id, uint32_t* start, uint32_t* length);