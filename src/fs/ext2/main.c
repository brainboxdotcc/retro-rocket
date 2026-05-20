#include <kernel.h>
#include <filesystem.h>
#include <ext2.h>

/* Singleton ext2 driver registered with the VFS */
static filesystem_t *ext2_fs = NULL;

static uint64_t ext2_block_to_lba(ext2_t *fs, uint32_t block) {
	uint64_t lba = fs->start + ((uint64_t)block * fs->block_size) / fs->device->block_size;
	uint64_t partition_end = fs->start + (fs->length / fs->device->block_size);
	if (lba >= partition_end) {
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return fs->start;
	}
	return lba;
}

static bool ext2_read_block(ext2_t *fs, uint32_t block, void *buffer) {
	return read_storage_device(fs->device->name, ext2_block_to_lba(fs, block), fs->block_size, buffer);
}

static bool ext2_read_inode(ext2_t *fs, uint32_t inode_num, ext2_inode_t *inode) {
	if (!inode_num) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}

	uint32_t group = (inode_num - 1) / fs->inodes_per_group;
	uint32_t index = (inode_num - 1) % fs->inodes_per_group;

	if (group >= fs->group_count) {
		fs_set_error(FS_ERR_OUT_OF_BOUNDS);
		return false;
	}

	ext2_group_desc_t *gd = &fs->groups[group];
	uint32_t offset = index * fs->inode_size;
	uint32_t block = gd->inode_table + (offset / fs->block_size);
	uint32_t block_offset = offset % fs->block_size;
	unsigned char buffer[fs->block_size];

	if (!ext2_read_block(fs, block, buffer)) {
		return false;
	}
	memcpy(inode, buffer + block_offset, sizeof(ext2_inode_t));
	return true;
}

static uint32_t ext2_inode_block(ext2_t *fs, ext2_inode_t *inode, uint32_t logical) {

	size_t block_entries = fs->block_size / sizeof(uint32_t);
	uint32_t level1[block_entries];
	uint32_t level2[block_entries];
	uint32_t level3[block_entries];
	uint32_t indirect[block_entries];

	if (logical < 12) {
		return inode->block[logical];
	}

	logical -= 12;

	if (logical < block_entries) {
		if (!ext2_read_block(fs, inode->block[12], indirect)) {
			return 0;
		}
		uint32_t result = indirect[logical];
		return result;
	}

	logical -= block_entries;

	if (logical < block_entries * block_entries) {
		if (!ext2_read_block(fs, inode->block[13], level1)) {
			return 0;
		}
		uint32_t idx1 = logical / block_entries;
		uint32_t idx2 = logical % block_entries;
		if (!ext2_read_block(fs, level1[idx1], level2)) {
			return 0;
		}
		uint32_t result = level2[idx2];
		return result;
	}

	logical -= block_entries * block_entries;

	if (!ext2_read_block(fs, inode->block[14], level1)) {
		return 0;
	}

	uint32_t span = block_entries * block_entries;
	uint32_t idx1 = logical / span;
	uint32_t rem = logical % span;
	uint32_t idx2 = rem / block_entries;
	uint32_t idx3 = rem % block_entries;

	if (!ext2_read_block(fs, level1[idx1], level2)) {
		return 0;
	}
	if (!ext2_read_block(fs, level2[idx2], level3)) {
		return 0;
	}

	uint32_t result = level3[idx3];
	return result;
}

static fs_directory_entry_t *ext2_parse_directory(fs_tree_t *node, ext2_t *fs, uint32_t inode_num) {
	ext2_inode_t inode;

	if (!ext2_read_inode(fs, inode_num, &inode)) {
		dprintf("ext2: Failed to read inode %d\n", inode_num);
		return NULL;
	}

	if (inode.size > 1024 * 1024 * 4) {
		dprintf("ext2: suspiciously large inode size %u\n", inode.size);
		fs_set_error(FS_ERR_OVERSIZED_DIRECTORY);
		return NULL;
	}

	unsigned char buffer[inode.size];
	uint32_t copied = 0;
	uint32_t logical = 0;

	while (copied < inode.size) {
		uint32_t phys = ext2_inode_block(fs, &inode, logical++);
		if (!phys) {
			break;
		}

		unsigned char blk[fs->block_size];

		if (!ext2_read_block(fs, phys, blk)) {
			dprintf("ext2: Failed to read block %d\n", phys);
			return NULL;
		}

		uint32_t remain = inode.size - copied;
		uint32_t amount = remain > fs->block_size ? fs->block_size : remain;

		memcpy(buffer + copied, blk, amount);
		copied += amount;
	}

	fs_directory_entry_t *list = NULL;
	unsigned char *walk = buffer;

	while (walk < buffer + inode.size) {
		ext2_dir_entry_t *entry = (ext2_dir_entry_t *)walk;

		if (!entry->rec_len) {
			break;
		}

		if (entry->inode && entry->name_len) {
			char filename[EXT2_MAX_FILE_NAME];
			uint8_t name_len = entry->name_len;
			memcpy(filename, entry->name, name_len);
			filename[name_len] = 0;

			if (strcmp(filename, ".") && strcmp(filename, "..")) {
				fs_directory_entry_t *item = kcalloc(1, sizeof(fs_directory_entry_t));
				if (!item) {
					dprintf("ext2: Out of memory allocating fs_directory_entry_t\n");
					fs_set_error(FS_ERR_OUT_OF_MEMORY);
					return NULL;
				}

				item->filename = strdup(filename);
				if (!item->filename) {
					dprintf("ext2: Out of memory allocating filename\n");
					kfree_null(&item);
					fs_set_error(FS_ERR_OUT_OF_MEMORY);
					return NULL;
				}
				item->lbapos = entry->inode;
				item->directory = node;
				strlcpy(item->device_name, fs->device->name, 16);

				ext2_inode_t child;
				if (ext2_read_inode(fs, entry->inode, &child)) {
					if ((child.mode & EXT2_S_IFMT) == EXT2_S_IFREG) {
						item->size = child.size;
					} else {
						/* Anything that isn't a regular file gets a size of 0.
						 * This stops users poking around inside file types that
						 * Retro Rocket does not support.
						 */
						item->size = 0;
					}
					datetime_t dt;
					datetime_from_time_t(child.mtime, &dt);
					item->year = dt.century * 100 + dt.year;
					item->month = dt.month;
					item->day = dt.day;
					item->hour = dt.hour;
					item->min = dt.minute;
					item->sec = dt.second;

					if ((child.mode & EXT2_S_IFMT) == EXT2_S_IFDIR) {
						item->flags |= FS_DIRECTORY;
					}
				}
				item->next = list;
				list = item;
			}
		}
		walk += entry->rec_len;
	}
	return list;
}

static void *ext2_get_directory(void *t) {
	fs_tree_t *treeitem = t;
	if (!treeitem) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}
	ext2_t *fs = (ext2_t *)treeitem->opaque;
	uint32_t inode = treeitem->lbapos ? treeitem->lbapos : EXT2_ROOT_INODE;
	return ext2_parse_directory(treeitem, fs, inode);
}

static bool ext2_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer) {
	fs_directory_entry_t *file = f;
	storage_device_t *dev = find_storage_device(file->device_name);

	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return false;
	}

	fs_tree_t *tree = file->directory;
	if (!tree) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	ext2_t *fs = (ext2_t *)tree->opaque;

	ext2_inode_t inode;
	if (!ext2_read_inode(fs, file->lbapos, &inode)) {
		return false;
	}

	if (start >= inode.size) {
		return true;
	}

	if (start + length > inode.size) {
		length = inode.size - start;
	}

	uint32_t done = 0;

	while (done < length) {
		uint64_t pos = start + done;
		uint32_t logical = pos / fs->block_size;
		uint32_t offset = pos % fs->block_size;
		uint32_t phys = ext2_inode_block(fs, &inode, logical);

		if (!phys) {
			break;
		}

		unsigned char blk[fs->block_size];

		if (!ext2_read_block(fs, phys, blk)) {
			return false;
		}

		uint32_t remain = length - done;
		uint32_t chunk = fs->block_size - offset;

		if (chunk > remain) {
			chunk = remain;
		}

		memcpy(buffer + done, blk + offset, chunk);
		done += chunk;
	}

	return true;
}

static ext2_t *ext2_mount_volume(const char *device) {
	storage_device_t *dev = find_storage_device(device);
	if (!dev) {
		dprintf("ext2: Invalid block device '%s'\n", device);
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}
	char found_guid[64] = {};
	uint64_t start = 0;
	uint64_t length = 0;

	ext2_t *fs = kcalloc(1, sizeof(ext2_t));
	if (!fs) {
		dprintf("ext2: Out of memory allocating fs struct\n");
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	fs->device = dev;

	if (!find_partition_of_type(device, PARTITION_LINUX_FILESYSTEM, found_guid, GPT_LINUX_FILESYSTEM, &fs->partitionid, &start, &length)) {
		fs->start = 0;
		fs->length = dev->size * dev->block_size;
	} else {
		fs->start = start;
		fs->length = length * dev->block_size;
		if (fs->partitionid != 0xFF) {
			dprintf("Found ext2 partition, device %s, MBR partition %d\n", device, fs->partitionid + 1);
		} else {
			dprintf("Found ext2 partition, device %s, GPT partition %s\n", device, found_guid);
		}

	}

	unsigned char buffer[2048];

	if (!read_storage_device(device, fs->start + (EXT2_SUPERBLOCK_OFFSET / dev->block_size), 2048, buffer)) {
		kfree_null(&fs);
		dprintf("ext2: Failed to read superblock at block offset %016lx\n", fs->start + (EXT2_SUPERBLOCK_OFFSET / dev->block_size));
		return NULL;
	}

	memcpy(&fs->superblock, buffer + (EXT2_SUPERBLOCK_OFFSET % dev->block_size), sizeof(ext2_superblock_t));

	if (fs->superblock.magic != EXT2_SUPER_MAGIC) {
		kfree_null(&fs);
		dprintf("ext2: Invalid magic, expected %08x, got %08x\n", EXT2_SUPER_MAGIC, fs->superblock.magic);
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	if (fs->superblock.feature_incompat & EXT2_INCOMPAT_COMPRESSION) {
		kfree_null(&fs);
		dprintf("ext2: Unsupported feature (compression)\n");
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	if (fs->superblock.feature_incompat & EXT2_INCOMPAT_RECOVER) {
		kfree_null(&fs);
		dprintf("ext2: Unsupported volume (requires fsck; not supported on Retro Rocket)\n");
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	if (fs->superblock.feature_incompat & EXT2_INCOMPAT_META_BG) {
		kfree_null(&fs);
		dprintf("ext2: Unsupported volume (meta block groups)\n");
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	if (fs->superblock.feature_incompat & EXT2_INCOMPAT_JOURNAL) {
		dprintf("ext2: Volume has a journal, likely ext3. Continuing anyway as the volume is clean.\n");
	}

	if (fs->superblock.log_block_size > 4) {
		kfree_null(&fs);
		dprintf("ext2: log_block_size is too large (%d)\n", fs->superblock.log_block_size);
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	fs->block_size = 1024 << fs->superblock.log_block_size;
	fs->inode_size = fs->superblock.inode_size ? fs->superblock.inode_size : 128;
	fs->blocks_per_group = fs->superblock.blocks_per_group;
	fs->inodes_per_group = fs->superblock.inodes_per_group;

	fs->group_count = (fs->superblock.blocks_count + fs->blocks_per_group - 1) / fs->blocks_per_group;
	if (fs->group_count > UINT32_MAX / sizeof(ext2_group_desc_t)) {
		dprintf("ext2: Pathologic group count\n");
		kfree_null(&fs);
		return NULL;
	}
	uint32_t gd_block = fs->block_size == 1024 ? 2 : 1;
	size_t gd_size = (size_t)fs->group_count * sizeof(ext2_group_desc_t);
	size_t gd_blocks = (gd_size + fs->block_size - 1) / fs->block_size;

	fs->groups = kcalloc(gd_blocks, fs->block_size);
	if (!fs->groups) {
		kfree_null(&fs);
		dprintf("ext2: Out of memory allocating blocks\n");
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	for (uint32_t i = 0; i < gd_blocks; ++i) {
		if (!ext2_read_block(fs, gd_block + i, ((unsigned char *)fs->groups) + (i * fs->block_size))) {
			dprintf("ext2: Failed to read blocks\n");
			kfree_null(&fs->groups);
			kfree_null(&fs);
			return NULL;
		}
	}

	strlcpy(fs->volume_name, fs->superblock.volume_name, MAX_VOLUME_NAME);
	fs->root = ext2_parse_directory(NULL, fs, EXT2_ROOT_INODE);
	if (!fs->root) {
		dprintf("ext2: Failed to read root directory: %s\n", fs_strerror(fs_get_error()));
		kfree_null(&fs->groups);
		kfree_null(&fs);
		return NULL;
	}

	return fs;
}

static int ext2_attach(const char *device, const char *path) {
	ext2_t *vol = ext2_mount_volume(device);
	if (!vol) {
		return 0;
	}
	return attach_filesystem(path, ext2_fs, vol);
}

void init_ext2() {
	ext2_fs = kcalloc(1, sizeof(filesystem_t));
	if (!ext2_fs) {
		return;
	}
	strlcpy(ext2_fs->name, "ext2", 31);
	ext2_fs->mount = ext2_attach;
	ext2_fs->getdir = ext2_get_directory;
	ext2_fs->readfile = ext2_read_file;
	register_filesystem(ext2_fs);
}
