/**
 * @file iso9660.h
 * @brief Structures and helpers for ISO9660 filesystem support.
 *
 * Provides the definitions for ISO9660 primary and supplementary
 * volume descriptors, directory entries, and related date formats.
 * Maps an ISO9660 filesystem to the Retro Rocket VFS layer.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

/** @brief LBA location of the Primary Volume Descriptor on an ISO9660 disc. */
#define PVD_LBA 0x10

/* ------------------------------------------------------------------------- */
/* ISO9660 structures                                                        */
/* ------------------------------------------------------------------------- */

/**
 * @brief High-level ISO9660 volume mapping.
 *
 * Wraps an ISO9660 filesystem into a linked list of VFS entries usable
 * by the kernel's virtual filesystem layer.
 */
typedef struct {
	int joliet;                    /**< Non-zero if Joliet extensions are in use */
	char* volume_name;             /**< Volume name string */
	uint32_t pathtable_lba;        /**< LBA of the path table */
	uint32_t rootextent_lba;       /**< LBA of the root directory extent */
	uint32_t rootextent_len;       /**< Length of the root directory extent */
	fs_directory_entry_t* root;    /**< Pointer to root directory in VFS */
	storage_device_t* device;      /**< Underlying storage device */
} iso9660;

/**
 * @brief Date structure as defined in the Primary Volume Descriptor (PVD).
 */
typedef struct {
	char year[4];          /**< Year (ASCII, 4 digits) */
	char month[2];         /**< Month (01–12) */
	char day[2];           /**< Day (01–31) */
	char hour[2];          /**< Hour (00–23) */
	char minute[2];        /**< Minute (00–59) */
	char second[2];        /**< Second (00–59) */
	char millisecond[2];   /**< Hundredths of a second */
	char tz_offset;        /**< Timezone offset from GMT in 15-minute intervals */
} __attribute__((packed)) PVD_date;

/**
 * @brief Compact per-file date entry format (differs from PVD_date).
 */
typedef struct {
	uint8_t years_since_1900; /**< Years since 1900 */
	uint8_t month;            /**< Month (1–12) */
	uint8_t day;              /**< Day (1–31) */
	uint8_t hour;             /**< Hour (0–23) */
	uint8_t minute;           /**< Minute (0–59) */
	uint8_t second;           /**< Second (0–59) */
	char tz_offset;           /**< Timezone offset from GMT in 15-minute intervals */
} __attribute__((packed)) DIRECTORY_date;

/**
 * @brief Directory entry (file or subdirectory).
 *
 * Filenames may exceed the fixed `filename[12]` buffer.
 * Walk the list of entries using `length`, which accounts
 * for variable-length names.
 */
typedef struct {
	uint8_t length;                 /**< Length of this directory entry */
	uint8_t attribute_length;       /**< Length of extended attributes */
	uint32_t extent_lba_lsb;        /**< File extent LBA (LSB) */
	uint32_t extent_lba_msb;        /**< File extent LBA (MSB) */
	uint32_t data_length_lsb;       /**< Data length (LSB) */
	uint32_t data_length_msb;       /**< Data length (MSB) */
	DIRECTORY_date recording_date;  /**< Recording timestamp */
	uint8_t file_flags;             /**< File flags (e.g., directory, hidden) */
	uint16_t interleave_unit_size;  /**< Interleave unit size (rarely used) */
	uint16_t sequence_number_lsb;   /**< File sequence number (LSB) */
	uint16_t sequence_number_msb;   /**< File sequence number (MSB) */
	uint8_t filename_length;        /**< Length of filename in bytes */
	char filename[12];              /**< Truncated filename (see note above) */
} __attribute__((packed)) ISO9660_directory;

/**
 * @brief Primary Volume Descriptor (PVD).
 *
 * Defines the metadata for the filesystem. Contains dual-endian fields
 * (LSB/MSB) for cross-platform compatibility.
 */
typedef struct {
	uint8_t typecode;                /**< Descriptor type (should be 1 for PVD) */
	char standardidentifier[5];      /**< "CD001" identifier */
	uint8_t version;                 /**< Version (always 1) */
	uint8_t unused;
	char systemidentifier[32];       /**< System identifier */
	char volumeidentifier[32];       /**< Volume identifier */
	char unused2[8];
	uint32_t lsb_volumespacesize;    /**< Volume space size (LSB) */
	uint32_t msb_volumespacesize;    /**< Volume space size (MSB) */
	char escape_seq[8];              /**< Escape sequences for Joliet */
	char unused3[24];                /**< Padding */
	uint16_t lsb_volumesetsize;      /**< Volume set size (LSB) */
	uint16_t msb_volumesetsize;      /**< Volume set size (MSB) */
	uint16_t lsb_volumeseqno;        /**< Volume sequence number (LSB) */
	uint16_t msb_volumeseqno;        /**< Volume sequence number (MSB) */
	uint16_t lsb_blocksize;          /**< Logical block size (LSB) */
	uint16_t msb_blocksize;          /**< Logical block size (MSB) */
	uint32_t lsb_pathtablesize;      /**< Path table size (LSB) */
	uint32_t msb_pathtablesize;      /**< Path table size (MSB) */
	uint32_t lsb_pathtable_L_lba;    /**< Path table L (LSB) */
	uint32_t lsb_optpathtable_L_lba; /**< Optional path table L (LSB) */
	uint32_t lsb_pathtable_M_lba;    /**< Path table M (MSB) */
	uint32_t lsb_optpathtable_M_lba; /**< Optional path table M (MSB) */
	ISO9660_directory root_directory;/**< Root directory entry */
	char volume_set_id[128];         /**< Volume set identifier */
	char publisher_id[128];          /**< Publisher identifier */
	char data_preparer[128];         /**< Data preparer identifier */
	char application_id[128];        /**< Application identifier */
	char copyright_file[38];         /**< Copyright file identifier */
	char abstract_file[36];          /**< Abstract file identifier */
	char bibliographic_file[37];     /**< Bibliographic file identifier */
	PVD_date volume_creation_date;   /**< Volume creation time */
	PVD_date volume_modification_date;/**< Volume modification time */
	PVD_date volume_expire_date;     /**< Volume expiry time */
	PVD_date volume_effective_date;  /**< Volume effective time */
	uint8_t file_structure_version;  /**< File structure version (1) */
	char unused4;
	uint8_t application_use[512];    /**< Application-specific use */
	uint8_t reserved[653];           /**< Reserved / padding */
} __attribute__((packed)) PVD;

/**
 * @brief Supplementary Volume Descriptor (SVD).
 *
 * Placeholder for Joliet or other extensions.
 */
typedef struct {
} __attribute__((packed)) SVD;

/* ------------------------------------------------------------------------- */
/* Functions                                                                 */
/* ------------------------------------------------------------------------- */

/**
 * @brief Initialise ISO9660 support and register with the VFS layer.
 */
void init_iso9660();
