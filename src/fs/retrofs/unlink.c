#include <kernel.h>
#include <retrofs.h>

bool rfs_delete_directory_entry(fs_directory_entry_t* file) {
	if (file == NULL || file->directory == NULL || file->filename == NULL) {
		dprintf("rfs: delete: bad args\n");
		return false;
	}

	fs_tree_t* tree = (fs_tree_t*)file->directory;
	rfs_t* info = (rfs_t*)tree->opaque;
	if (info == NULL || info->desc == NULL) {
		dprintf("rfs: delete: missing fs context\n");
		return false;
	}

	const char* target = file->filename;
	if (target[0] == '\0') {
		dprintf("rfs: delete: empty name\n");
		return false;
	}

	uint64_t current = (tree->lbapos != 0) ? tree->lbapos : info->desc->root_directory;

	/* One directory block buffer (64 sectors -> 128 half-sector entries) */
	rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];

	/* Corruption guard */
	uint32_t walked = 0;
	const uint32_t walk_limit = 1u << 16;

	while (current != 0) {
		if (walked++ > walk_limit) {
			dprintf("rfs: delete: aborting (cycle suspected)\n");
			return false;
		}

		if (!rfs_read_device(info, current, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("rfs: delete: read error at %lu\n", current);
			return false;
		}

		rfs_directory_start_t* start = (rfs_directory_start_t*)&block[0];
		if ((start->flags & RFS_FLAG_DIR_START) == 0 || start->sectors != RFS_DEFAULT_DIR_SIZE) {
			dprintf("rfs: delete: invalid dir block at %lu\n", current);
			return false;
		}

		/* Treat entries as half-sector units to safely memmove (due to flexible array member). */
		rfs_directory_entry_t* ents = (rfs_directory_entry_t*)&block[0];

		/* Scan to find the matching entry and the last used index in this block. */
		const size_t entries_per_block = (size_t)((RFS_DEFAULT_DIR_SIZE * 2) - 1);
		size_t found_idx = (size_t)-1;
		size_t last_used = 0;

		for (size_t i = 1; i <= entries_per_block; i++) {
			rfs_directory_entry_inner_t* e = &ents[i].entry;

			if (e->filename[0] == '\0') {
				/* We’ve hit the terminator; i-1 is the last used entry. */
				last_used = (i >= 1) ? (i - 1) : 0;
				break;
			}

			last_used = i;

			/* Defensive: ensure there is a NUL terminator somewhere in the field. */
			/* No strnlen() in your libc; we rely on well-formed entries as produced by your code. */

			if (found_idx == (size_t)-1) {
				if (strcasecmp(e->filename, target) == 0) {
					found_idx = i;
					/* Do not break; continue to locate last_used precisely. */
				}
			}
		}

		if (found_idx != (size_t)-1) {
			/* Compact: shift following entries left by one within this block, then clear the tail slot. */
			if (found_idx < last_used) {
				size_t move_count = last_used - found_idx;
				memmove(&ents[found_idx], &ents[found_idx + 1], move_count * sizeof(rfs_directory_entry_t));
			}

			/* Clear the last_used slot (now duplicated or original tail). */
			memset(&ents[last_used], 0, sizeof(rfs_directory_entry_t));

			/* Persist the modified block.  NOTE: if rfs_write_device is int, change this check accordingly. */
			if (!rfs_write_device(info, current, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				dprintf("rfs: delete: write error at %lu\n", current);
				return false;
			}

			/* Policy: we deliberately do not unlink empty continuation blocks. */
			return true;
		}

		if (start->continuation == 0) {
			break;
		}

		current = start->continuation;
	}

	/* Not found anywhere in the chain. */
	return false;
}



bool rfs_unlink_file(void* dir, const char* name) {
	if (dir == NULL || name == NULL || name[0] == '\0') {
		dprintf("rfs: unlink_file: bad args\n");
		return false;
	}

	fs_tree_t* tree = (fs_tree_t*)dir;
	rfs_t* info = (rfs_t*)tree->opaque;
	if (info == NULL || info->desc == NULL) {
		dprintf("rfs: unlink_file: missing fs context\n");
		return false;
	}

	/* Locate the entry in VFS form, so we can pass it to delete */
	fs_directory_entry_t target = { 0 };
	target.directory = tree;
	target.filename  = (char*)name;

	/* Before deleting, we need the on-disk entry to get sector_start/length */
	uint64_t blk_sector = 0;
	size_t   blk_index = 0;
	rfs_directory_entry_inner_t on_disk;
	if (!rfs_locate_entry(info, tree, name, &blk_sector, &blk_index, &on_disk)) {
		dprintf("rfs: unlink_file: '%s' not found\n", name);
		return false;
	}

	if ((on_disk.flags & RFS_FLAG_DIRECTORY) != 0) {
		dprintf("rfs: unlink_file: '%s' is a directory\n", name);
		return false;
	}

	/* Now just call the existing delete */
	bool done = rfs_delete_directory_entry(&target);
	if (done && on_disk.sector_length > 0) {
		if (!rfs_mark_extent(info, on_disk.sector_start, on_disk.sector_length, false)) {
			dprintf("rfs: unlink_file: failed to free data extent for '%s'\n", name);
			return false;
		}
	}
	return done;
}

