#pragma once

#include <kernel.h>
#include <filesystem.h>

/**
 * @brief Offset in bytes to the ext2 superblock from the start of the volume.
 *
 * The primary ext2 superblock is traditionally located 1024 bytes from the
 * start of the filesystem regardless of block size.
 */
#define EXT2_SUPERBLOCK_OFFSET 1024

/**
 * @brief ext2/ext3/ext4 superblock magic value.
 */
#define EXT2_SUPER_MAGIC 0xEF53

/**
 * @brief Root directory inode number.
 *
 * The root directory of all ext-family filesystems is inode 2.
 */
#define EXT2_ROOT_INODE 2

/**
 * @brief Compression incompatibility feature bit.
 *
 * Retro Rocket does not support compressed ext volumes.
 */
#define EXT2_INCOMPAT_COMPRESSION 0x0001

/**
 * @brief Journal recovery required incompatibility feature bit.
 *
 * Indicates that the filesystem requires journal replay before metadata can
 * safely be considered consistent.
 */
#define EXT2_INCOMPAT_RECOVER 0x0004

/**
 * @brief Journal present incompatibility feature bit.
 *
 * Indicates that the filesystem contains a journal and is likely ext3.
 * Retro Rocket ignores the journal on clean readonly volumes.
 */
#define EXT2_INCOMPAT_JOURNAL 0x0008

/**
 * @brief Meta block group incompatibility feature bit.
 *
 * Retro Rocket does not currently support meta block groups.
 */
#define EXT2_INCOMPAT_META_BG 0x0010

/**
 * @brief Mask used to extract inode file type bits from inode mode.
 */
#define EXT2_S_IFMT 0xF000

/**
 * @brief Inode type value for directories.
 */
#define EXT2_S_IFDIR 0x4000

/**
 * @brief Inode type value for regular files.
 */
#define EXT2_S_IFREG 0x8000

/**
 * @brief Maximum ext volume label length.
 *
 * ext volume labels are fixed-width fields and are not guaranteed to be
 * null-terminated on disk.
 */
#define MAX_VOLUME_NAME 16

/**
 * @brief Maximum supported filename length.
 *
 * ext directory entry names are limited to 255 bytes.
 * One additional byte is reserved for null termination.
 */
#define EXT2_MAX_FILE_NAME 256

/**
 * @brief ext2/ext3/ext4 superblock structure.
 *
 * This structure is read directly from disk and describes the overall
 * geometry and feature flags of the filesystem.
 */
typedef struct ext2_superblock_t {
	uint32_t inodes_count;			/**< Total inode count */
	uint32_t blocks_count;			/**< Total block count */
	uint32_t r_blocks_count;		/**< Reserved block count */
	uint32_t free_blocks_count;		/**< Free block count */
	uint32_t free_inodes_count;		/**< Free inode count */
	uint32_t first_data_block;		/**< First data block */
	uint32_t log_block_size;		/**< log2(block size / 1024) */
	int32_t log_frag_size;			/**< Fragment size shift */
	uint32_t blocks_per_group;		/**< Blocks per block group */
	uint32_t frags_per_group;		/**< Fragments per block group */
	uint32_t inodes_per_group;		/**< Inodes per block group */
	uint32_t mtime;				/**< Last mount time */
	uint32_t wtime;				/**< Last write time */
	uint16_t mount_count;			/**< Mount count since last check */
	uint16_t max_mount_count;		/**< Maximum mounts before fsck */
	uint16_t magic;				/**< Superblock magic value */
	uint16_t state;				/**< Filesystem state */
	uint16_t errors;			/**< Error handling behaviour */
	uint16_t minor_rev_level;		/**< Minor revision level */
	uint32_t lastcheck;			/**< Last filesystem check time */
	uint32_t checkinterval;			/**< Maximum interval between checks */
	uint32_t creator_os;			/**< Creating operating system */
	uint32_t rev_level;			/**< Revision level */
	uint16_t def_resuid;			/**< Default reserved UID */
	uint16_t def_resgid;			/**< Default reserved GID */
	uint32_t first_ino;			/**< First non-reserved inode */
	uint16_t inode_size;			/**< Size of each inode */
	uint16_t block_group_nr;		/**< Block group number of this superblock */
	uint32_t feature_compat;		/**< Compatible feature flags */
	uint32_t feature_incompat;		/**< Incompatible feature flags */
	uint32_t feature_ro_compat;		/**< Readonly compatible feature flags */
	uint8_t uuid[16];			/**< Filesystem UUID */
	char volume_name[MAX_VOLUME_NAME];	/**< Volume label */
	char last_mounted[64];			/**< Last mounted path */
} __attribute__((packed)) ext2_superblock_t;

/**
 * @brief ext2 block group descriptor.
 *
 * Describes the location of inode tables and allocation bitmaps for a block
 * group.
 */
typedef struct ext2_group_desc_t {
	uint32_t block_bitmap;		/**< Block bitmap block number */
	uint32_t inode_bitmap;		/**< Inode bitmap block number */
	uint32_t inode_table;		/**< Starting block of inode table */
	uint16_t free_blocks_count;	/**< Free blocks in group */
	uint16_t free_inodes_count;	/**< Free inodes in group */
	uint16_t used_dirs_count;	/**< Number of directories in group */
	uint16_t pad;			/**< Alignment padding */
	uint8_t reserved[12];		/**< Reserved */
} __attribute__((packed)) ext2_group_desc_t;

/**
 * @brief ext-family inode structure.
 *
 * Inodes describe files, directories, symlinks and special filesystem
 * objects.
 */
typedef struct ext2_inode_t {
	uint16_t mode;			/**< File type and permissions */
	uint16_t uid;			/**< Owner UID */
	uint32_t size;			/**< File size in bytes */
	uint32_t atime;			/**< Last access time */
	uint32_t ctime;			/**< Creation/change time */
	uint32_t mtime;			/**< Last modification time */
	uint32_t dtime;			/**< Deletion time */
	uint16_t gid;			/**< Owner GID */
	uint16_t links_count;		/**< Hard link count */
	uint32_t blocks;		/**< Number of 512-byte sectors */
	uint32_t flags;			/**< Inode flags */
	uint32_t osd1;			/**< OS-dependent value */
	uint32_t block[15];		/**< Direct and indirect block pointers */
	uint32_t generation;		/**< File version */
	uint32_t file_acl;		/**< File ACL block */
	uint32_t dir_acl;		/**< Directory ACL or upper size */
	uint32_t faddr;			/**< Fragment address */
	uint8_t osd2[12];		/**< OS-dependent value */
} __attribute__((packed)) ext2_inode_t;

/**
 * @brief ext-family directory entry.
 *
 * Directories in ext filesystems are flat collections of variable-length
 * directory entry records.
 */
typedef struct ext2_dir_entry_t {
	uint32_t inode;			/**< Referenced inode number */
	uint16_t rec_len;		/**< Length of directory record */
	uint8_t name_len;		/**< Filename length */
	uint8_t file_type;		/**< Directory entry file type */
	char name[];			/**< Filename bytes */
} __attribute__((packed)) ext2_dir_entry_t;

/**
 * @brief Per-mounted-volume ext filesystem state.
 *
 * This structure is attached to mounted VFS trees via fs_tree_t::opaque and
 * stores filesystem geometry and mount-specific metadata.
 */
typedef struct ext2_t {
	storage_device_t *device;		/**< Backing storage device */
	uint64_t start;				/**< Starting LBA of filesystem */
	uint64_t length;			/**< Length of filesystem in bytes */
	uint8_t partitionid;			/**< Partition identifier */
	char volume_name[MAX_VOLUME_NAME];	/**< Cached volume label */
	uint32_t block_size;			/**< Filesystem block size */
	uint32_t inode_size;			/**< Size of each inode */
	uint32_t blocks_per_group;		/**< Blocks per block group */
	uint32_t inodes_per_group;		/**< Inodes per block group */
	uint32_t group_count;			/**< Total number of block groups */
	ext2_superblock_t superblock;		/**< Cached superblock */
	ext2_group_desc_t *groups;		/**< Cached group descriptor table */
	fs_directory_entry_t *root;		/**< Cached root directory */
} ext2_t;

/**
 * @brief Linux filesystem partition GUID.
 *
 * Used to identify Linux native partitions on GPT partitioned disks.
 */
#define GPT_LINUX_FILESYSTEM "0FC63DAF-8483-4772-8E79-3D69D8477DE4"

/**
 * @brief Linux native partition type for MBR partition tables.
 */
#define PARTITION_LINUX_FILESYSTEM 0x83

/**
 * @brief Initialise and register the ext2 filesystem driver.
 *
 * Registers readonly ext2/ext3-compatible support with the VFS layer.
 */
void init_ext2();