#include <kernel.h>
#include <retrofs.h>

filesystem_t *rfs_fs = NULL;


bool read_rfs_description_block(rfs_t *info) {
	if (!info) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	rfs_description_block_padded_t *description_block = kmalloc(sizeof(rfs_description_block_padded_t));
	if (!description_block) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	if (!rfs_read_device(info, 0, RFS_SECTOR_SIZE, description_block)) {
		kprintf("RFS: Could not read RFS description block!\n");
		kfree_null(&description_block);
		return false;
	}
	if (description_block->desc.identifier != RFS_ID) {
		dprintf("Identifier %lx is not %lx (\"RetroFS1\")\n", description_block->desc.identifier, RFS_ID);
		kfree_null(&description_block);
		return false;
	}
	info->desc = &description_block->desc;

	return true;
}

rfs_t *rfs_mount_volume(const char *device_name) {
	if (!device_name) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}
	char found_guid[64];
	rfs_t *info = kmalloc(sizeof(rfs_t));
	if (!info) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	info->dev = find_storage_device(device_name);
	if (!info->dev || info->dev->block_size != RFS_SECTOR_SIZE) {
		kprintf("%s: Device not suitable for RFS\n", info->dev ? info->dev->name : "<unknown>");
		kfree_null(&info);
		return 0;
	}

	uint64_t start = 0, length = 0;

	bool success = false;
	uint8_t partitionid = 0;
	if (!find_partition_of_type(device_name, 0xFF, found_guid, RFS_GPT_GUID, &partitionid, &start, &length)) {
		/* No partition found, attempt to mount the volume as whole disk */
		info->start = 0;
		info->length = info->dev->size * info->dev->block_size;
		success = read_rfs_description_block(info);
		if (success) {
			kprintf("Found RFS volume, device %s\n", device_name);
		}
	} else {
		info->start = start;
		info->length = length * info->dev->block_size;
		/* 0xFF means we got a GPT partition not MBR partition; only GPT partitions are supported for RFS. */
		if (partitionid == 0xFF) {
			success = read_rfs_description_block(info);
			if (success) {
				kprintf("Found RFS GPT partition, device %s\n", device_name);
			}
		}
	}
	if (!success) {
		kfree_null(&info);
		return NULL;
	}
	if (!rfs_build_level_caches(info)) {
		dprintf("Failed to build RFS L1/L2 caches\n");
		kfree_null(&info);
		return NULL;
	}
	return info;
}


int rfs_attach(const char *device_name, const char *path) {
	rfs_t *rfs = rfs_mount_volume(device_name);
	if (rfs) {
		int success = attach_filesystem(path, rfs_fs, rfs);
		if (success) {
			dprintf("RFS: volume free space on '%s': %lu\n", path, fs_get_free_space(path));
		}
		return success;
	}
	fs_set_error(FS_ERR_NOT_RFS);
	return 0;
}

void init_rfs() {
	rfs_fs = kmalloc(sizeof(filesystem_t));
	if (!rfs_fs) {
		return;
	}
	strlcpy(rfs_fs->name, "rfs", 31);
	rfs_fs->mount = rfs_attach;
	rfs_fs->getdir = rfs_get_directory;
	rfs_fs->readfile = rfs_read_file;
	rfs_fs->createfile = rfs_create_file;
	rfs_fs->truncatefile = rfs_truncate_file;
	rfs_fs->createdir = rfs_create_directory;
	rfs_fs->writefile = rfs_write_file;
	rfs_fs->rm = rfs_unlink_file;
	rfs_fs->rmdir = rfs_unlink_dir;
	rfs_fs->freespace = rfs_get_free_space;
	register_filesystem(rfs_fs);
}