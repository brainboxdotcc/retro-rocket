#include <kernel.h>
#include <retrofs.h>

bool rfs_unlink_dir(void *dir, const char *name) {
	if (!dir || !name || !*name) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}

	fs_tree_t *tree = (fs_tree_t *) dir;
	rfs_t *info = (rfs_t *) tree->opaque;
	if (!info || !info->desc) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	/* Locate the child entry in the parent directory */
	uint64_t blk_sector = 0;
	size_t blk_index = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, name, &blk_sector, &blk_index, &ent)) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		return false;
	}
	if ((ent.flags & RFS_FLAG_DIRECTORY) == 0) {
		fs_set_error(FS_ERR_NOT_A_DIRECTORY);
		return false;
	}

	/* Check the directory (and all continuations) is empty */
	{
		uint64_t cur = ent.sector_start;
		uint32_t walked = 0;
		const uint32_t walk_limit = 1u << 16;

		while (cur != 0) {
			if (walked++ > walk_limit) {
				fs_set_error(FS_ERR_CYCLE_DETECTED);
				return false;
			}

			rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];
			if (!rfs_read_device(info, cur, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				return false;
			}

			rfs_directory_start_t *start_entry = (rfs_directory_start_t *) &block[0];
			if ((start_entry->flags & RFS_FLAG_DIR_START) == 0 ||
			    start_entry->sectors != RFS_DEFAULT_DIR_SIZE) {
				fs_set_error(FS_ERR_BROKEN_DIRECTORY);
				return false;
			}

			rfs_directory_entry_t *files = (rfs_directory_entry_t *) &block[1];
			const size_t entries_per_block = (RFS_DEFAULT_DIR_SIZE * 2) - 1;

			for (size_t i = 0; i < entries_per_block; i++) {
				if (files[i].entry.filename[0] == 0) {
					break; /* end of entries in this block */
				}
				fs_set_error(FS_ERR_DIRECTORY_NOT_EMPTY);
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
		fs_directory_entry_t target = (fs_directory_entry_t) {0};
		target.directory = tree;
		target.filename = (char *) name;

		if (!rfs_delete_directory_entry(&target)) {
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
				fs_set_error(FS_ERR_CYCLE_DETECTED);
				break;
			}

			rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];
			if (!rfs_read_device(info, cur, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				break;
			}

			rfs_directory_start_t *start_entry = (rfs_directory_start_t *) &block[0];
			uint64_t next = start_entry->continuation;
			uint64_t span = start_entry->sectors;

			rfs_mark_extent(info, cur, span, false);

			cur = next;
		}
	}

	return true;
}
