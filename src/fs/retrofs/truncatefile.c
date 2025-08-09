#include <kernel.h>
#include <retrofs.h>

bool rfs_truncate_file(void *f, size_t length) {
	if (!f) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}

	fs_directory_entry_t *file = (fs_directory_entry_t *) f;
	if (file->flags & FS_DIRECTORY) {
		fs_set_error(FS_ERR_NOT_A_FILE);
		return false;
	}

	fs_tree_t *tree = (fs_tree_t *) file->directory;
	if (!tree) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}
	rfs_t *info = (rfs_t *) tree->opaque;
	if (!info || !info->desc) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	/* Look up on-disk entry for reservation (sector_length). */
	uint64_t blk_sector = 0;
	size_t blk_index = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, file->filename, &blk_sector, &blk_index, &ent)) {
		fs_set_error(FS_ERR_NO_SUCH_FILE);
		return false;
	}
	if (ent.flags & RFS_FLAG_DIRECTORY) {
		fs_set_error(FS_ERR_NOT_A_FILE);
		return false;
	}

	/* Enforce reservation: truncate may grow size but NOT allocate. */
	const uint64_t reserved_bytes = ent.sector_length * RFS_SECTOR_SIZE;
	if (length > reserved_bytes) {
		fs_set_error(FS_ERR_TRUNCATE_BEYOND_LENGTH);
		dprintf("rfs: truncate_file: requested %lu > reserved %lu\n", length, reserved_bytes);
	}

	/* No-op if unchanged. */
	if (length == ent.length) {
		return true;
	}

	/* Update on-disk size; keep sector_length as-is. */
	fs_directory_entry_t de = (fs_directory_entry_t) {0};
	de.directory = tree;
	de.filename = file->filename;
	de.lbapos = file->lbapos;          /* unchanged */
	de.flags = 0;                     /* file */
	de.size = length;      /* new logical size */

	if (!rfs_upsert_directory_entry(&de, ent.sector_length)) {
		fs_set_error(FS_ERR_BROKEN_DIRECTORY);
		return false;
	}

	/* Reflect in VFS node. */
	file->size = length;
	return true;
}

