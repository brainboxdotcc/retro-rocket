#include <kernel.h>
#include <retrofs.h>

/**
 * Given a directory pointed to by `start_sector`, walk its sectors and enumerate its files,
 * building an fs_directory_entry_t* for the VFS.
 * @param tree VFS tree
 * @param info RFS info structure
 * @param start_sector Starting sector of directory
 * @return fs_directory_entry_t* or NULL if unable to parse
 */
fs_directory_entry_t* rfs_walk_directory(fs_tree_t* tree, rfs_t* info, uint64_t start_sector) {
	rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2]; // Each holds up to 2 files
	fs_directory_entry_t* list = NULL;

	if (!rfs_read_device(info, start_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block)) {
		return NULL;
	}

	rfs_directory_start_t* start_entry = (rfs_directory_start_t*)&block[0];
	rfs_directory_entry_t* files = (rfs_directory_entry_t*)&block[1];
	if (start_entry->sectors != RFS_DEFAULT_DIR_SIZE || (start_entry->flags & RFS_FLAG_DIR_START) == 0) {
		dprintf("Directory block has bogus values\n");
		return NULL;
	}

	/* Loop guard to avoid cycles in a corrupted continuation chain */
	uint32_t walked = 0;
	const uint32_t walk_limit = 1u << 16;

	do {
		if (walked++ > walk_limit) {
			dprintf("rfs: rfs_walk_directory: aborting (cycle suspected)\n");
			break;
		}

		for (size_t entry_index = 0; entry_index < RFS_DEFAULT_DIR_SIZE * 2 - 1; ++entry_index) {
			rfs_directory_entry_inner_t* entry = &files[entry_index].entry;
			if (entry->filename[0] == 0) {
				/* End of list */
				break;
			}
			/* Process entry */
			fs_directory_entry_t* file = kmalloc(sizeof(fs_directory_entry_t));
			file->filename = strdup(entry->filename);
			file->alt_filename = strdup(entry->filename);
			file->lbapos = entry->sector_start;
			file->directory = tree;
			file->flags = 0;
			file->size = entry->length;
			if (entry->flags & RFS_FLAG_DIRECTORY) {
				file->flags |= FS_DIRECTORY;
			}
			file->next = list;
			list = file;
		}
		if (start_entry->continuation == 0) {
			/* No continuations left */
			break;
		} else {
			if (!rfs_read_device(info, start_entry->continuation, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block)) {
				dprintf("rfs: Error reading next directory chain from %lu", start_entry->continuation);
				return list;
			}
			start_entry = (rfs_directory_start_t*)&block[0];
			files = (rfs_directory_entry_t*)&block[1];
			if (start_entry->sectors != RFS_DEFAULT_DIR_SIZE) {
				dprintf("Directory block has unexpected length of %lu\n", start_entry->sectors);
				return list;
			}
		}
	} while (true);
	return list;
}

void* rfs_get_directory(void* t) {
	if (!t) {
		dprintf("*** BUG *** rfs_get_directory: null fs_tree_t*!\n");
		return NULL;
	}
	fs_tree_t* tree = (fs_tree_t*)t;
	rfs_t* info = (rfs_t*)tree->opaque;
	return rfs_walk_directory(tree, info, tree->lbapos ? tree->lbapos : info->desc->root_directory);

}
