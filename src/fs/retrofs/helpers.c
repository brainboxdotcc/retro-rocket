#include <kernel.h>
#include <retrofs.h>

bool rfs_read_device(rfs_t *rfs, uint64_t start_sectors, uint64_t size_bytes, void *buffer) {
	const uint64_t total_sectors = rfs->total_sectors;
	const uint64_t nsectors = size_bytes / RFS_SECTOR_SIZE;

	if ((size_bytes % RFS_SECTOR_SIZE) != 0) {
		dprintf("rfs_read_device: size not sector-aligned\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}
	if (start_sectors + nsectors > total_sectors) {
		dprintf("rfs_read_device: would read past end of device\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}

	uint64_t volume_start = rfs->start;
	return read_storage_device(rfs->dev->name, volume_start + start_sectors, size_bytes, buffer);
}

bool rfs_write_device(rfs_t *rfs, uint64_t start_sectors, uint64_t size_bytes, const void *buffer) {
	const uint64_t total_sectors = rfs->total_sectors;
	const uint64_t nsectors = size_bytes / RFS_SECTOR_SIZE;

	if ((size_bytes % RFS_SECTOR_SIZE) != 0) {
		dprintf("rfs_write_device: size not sector-aligned\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}
	if (start_sectors + nsectors > total_sectors) {
		dprintf("rfs_write_device: would write past end of device\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}

	uint64_t volume_start = rfs->start;
	return write_storage_device(rfs->dev->name, volume_start + start_sectors, size_bytes, buffer);
}

bool rfs_clear_device(rfs_t *rfs, uint64_t start_sectors, uint64_t size_bytes) {
	const uint64_t total_sectors = rfs->total_sectors;
	const uint64_t nsectors = size_bytes / RFS_SECTOR_SIZE;

	if ((size_bytes % RFS_SECTOR_SIZE) != 0) {
		dprintf("rfs_clear_device: size not sector-aligned\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}
	if (start_sectors + nsectors > total_sectors) {
		dprintf("rfs_clear_device: would write past end of device\n");
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return 0;
	}

	uint64_t volume_start = rfs->start;
	return storage_device_clear_blocks(rfs->dev, volume_start + start_sectors, size_bytes);
}


bool rfs_locate_entry(rfs_t *info, fs_tree_t *tree, const char *name, uint64_t *out_sector, size_t *out_index, rfs_directory_entry_inner_t *out_entry_copy) {
	if (info == NULL || tree == NULL || name == NULL || name[0] == '\0') {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}

	uint64_t current = (tree->lbapos != 0) ? tree->lbapos : info->desc->root_directory;
	rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];

	uint32_t walked = 0;
	const uint32_t walk_limit = 1u << 16;

	while (current != 0) {
		if (walked++ > walk_limit) {
			dprintf("rfs: locate: cycle suspected\n");
			fs_set_error(FS_ERR_CYCLE_DETECTED);
			return false;
		}

		if (!rfs_read_device(info, current, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("rfs: locate: read error at %lu\n", current);
			return false;
		}

		rfs_directory_start_t *start = (rfs_directory_start_t *) &block[0];
		if ((start->flags & RFS_FLAG_DIR_START) == 0 || start->sectors != RFS_DEFAULT_DIR_SIZE) {
			dprintf("rfs: locate: invalid dir block at %lu\n", current);
			return false;
		}

		rfs_directory_entry_t *ents = (rfs_directory_entry_t *) &block[1];
		const size_t entries_per_block = (RFS_DEFAULT_DIR_SIZE * 2) - 1;

		for (size_t i = 0; i < entries_per_block; i++) {
			rfs_directory_entry_inner_t *e = &ents[i].entry;

			if (e->filename[0] == '\0') {
				break;
			}

			if (strcasecmp(e->filename, name) == 0) {
				if (out_sector != NULL) {
					*out_sector = current;
				}
				if (out_index != NULL) {
					*out_index = i;
				}
				if (out_entry_copy != NULL) {
					*out_entry_copy = *e;
				}
				return true;
			}
		}

		if (start->continuation == 0) {
			break;
		}
		current = start->continuation;
	}

	return false;
}

uint64_t rfs_get_free_space(void *fs) {
	fs_tree_t *tree = (fs_tree_t *) fs;
	if (!tree) {
		return 0;
	}
	rfs_t *info = tree->opaque;
	return (info->length * RFS_SECTOR_SIZE) - rfs_get_used_bytes(info);
}

uint64_t rfs_get_used_bytes(rfs_t *info) {
	if (!info || !info->desc) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}

	const uint64_t total = info->total_sectors;
	uint64_t free_secs = 0;

	/* Sum free sectors across all L1 groups (tail group handled by builder/marking) */
	for (uint64_t g = 0; g < info->l1_groups; ++g) {
		free_secs += info->l1_free_count[g];
	}

	/* Defensive clamp */
	if (free_secs > total) {
		free_secs = total;
	}

	const uint64_t used_secs = total - free_secs;
	return used_secs * (uint64_t) RFS_SECTOR_SIZE;
}
