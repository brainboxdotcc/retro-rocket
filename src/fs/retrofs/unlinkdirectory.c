#include <kernel.h>
#include <retrofs.h>

bool rfs_unlink_dir(void* dir, const char* name) {
	if (!dir || !name || !*name) {
		dprintf("rfs: unlink_dir: bad args\n");
		return false;
	}

	fs_tree_t* tree = (fs_tree_t*)dir;
	rfs_t* info = (rfs_t*)tree->opaque;
	if (!info || !info->desc) {
		dprintf("rfs: unlink_dir: missing fs context\n");
		return false;
	}

	/* Locate the child entry in the parent directory */
	uint64_t blk_sector = 0;
	size_t   blk_index  = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, name, &blk_sector, &blk_index, &ent)) {
		dprintf("rfs: unlink_dir: '%s' not found in '%s'\n",
			name, tree->name ? tree->name : "(anon)");
		return false;
	}
	if ((ent.flags & RFS_FLAG_DIRECTORY) == 0) {
		dprintf("rfs: unlink_dir: '%s' is not a directory\n", name);
		return false;
	}

	/* Check the directory (and all continuations) is empty */
	{
		uint64_t cur = ent.sector_start;
		uint32_t walked = 0;
		const uint32_t walk_limit = 1u << 16;

		while (cur != 0) {
			if (walked++ > walk_limit) {
				dprintf("rfs: unlink_dir: abort (cycle suspected)\n");
				return false;
			}

			rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];
			if (!rfs_read_device(info, cur, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				dprintf("rfs: unlink_dir: read error at %lu\n", cur);
				return false;
			}

			rfs_directory_start_t* start_entry = (rfs_directory_start_t*)&block[0];
			if ((start_entry->flags & RFS_FLAG_DIR_START) == 0 ||
			    start_entry->sectors != RFS_DEFAULT_DIR_SIZE) {
				dprintf("rfs: unlink_dir: invalid dir block at %lu\n", cur);
				return false;
			}

			rfs_directory_entry_inner_t* files = (rfs_directory_entry_inner_t*)&block[1];
			const size_t entries_per_block = (RFS_DEFAULT_DIR_SIZE * 2) - 1;

			for (size_t i = 0; i < entries_per_block; i++) {
				if (files[i].filename[0] == 0) {
					break; /* end of entries in this block */
				}
				dprintf("rfs: unlink_dir: not empty (found '%s')\n", files[i].filename);
				return false;
			}

			if (start_entry->continuation == 0) {
				break;
			}
			cur = start_entry->continuation;
		}
	}

	/* Remove the parent directory entry first (fail-safe ordering) */
	{
		fs_directory_entry_t target = (fs_directory_entry_t){0};
		target.directory = tree;
		target.filename  = (char*)name;

		if (!rfs_delete_directory_entry(&target)) {
			dprintf("rfs: unlink_dir: failed to remove parent entry for '%s'\n", name);
			return false;
		}
	}

	/* Free all blocks in the directory chain */
	{
		uint64_t cur = ent.sector_start;
		uint32_t walked = 0;
		const uint32_t walk_limit = 1u << 16;

		while (cur != 0) {
			if (walked++ > walk_limit) {
				dprintf("rfs: unlink_dir: abort during free (cycle suspected)\n");
				break;
			}

			rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];
			if (!rfs_read_device(info, cur, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				dprintf("rfs: unlink_dir: read error freeing at %lu\n", cur);
				break;
			}

			rfs_directory_start_t* start_entry = (rfs_directory_start_t*)&block[0];
			uint64_t next = start_entry->continuation;
			uint64_t span = start_entry->sectors;

			if (!rfs_mark_extent(info, cur, span, false)) {
				dprintf("rfs: unlink_dir: WARNING: failed to free %lu@%lu\n", span, cur);
				/* continue freeing the rest anyway */
			}

			cur = next;
		}
	}

	return true;
}
