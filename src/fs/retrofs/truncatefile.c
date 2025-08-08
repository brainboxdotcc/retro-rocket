#include <kernel.h>
#include <retrofs.h>

bool rfs_truncate_file(void* f, size_t length) {
	if (!f) {
		dprintf("rfs: truncate_file: null file\n");
		return false;
	}

	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	if (file->flags & FS_DIRECTORY) {
		dprintf("rfs: truncate_file: '%s' is a directory\n", file->filename ? file->filename : "(unnamed)");
		return false;
	}

	fs_tree_t* tree = (fs_tree_t*)file->directory;
	if (!tree) {
		dprintf("rfs: truncate_file: missing VFS tree\n");
		return false;
	}
	rfs_t* info = (rfs_t*)tree->opaque;
	if (!info || !info->desc) {
		dprintf("rfs: truncate_file: missing fs context\n");
		return false;
	}

	/* Look up on-disk entry for reservation (sector_length). */
	uint64_t blk_sector = 0;
	size_t   blk_index  = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, file->filename, &blk_sector, &blk_index, &ent)) {
		dprintf("rfs: truncate_file: locate failed for '%s'\n", file->filename ? file->filename : "(unnamed)");
		return false;
	}
	if (ent.flags & RFS_FLAG_DIRECTORY) {
		dprintf("rfs: truncate_file: '%s' is a directory (on-disk)\n", file->filename ? file->filename : "(unnamed)");
		return false;
	}

	/* Enforce reservation: truncate may grow size but NOT allocate. */
	const uint64_t reserved_bytes = ent.sector_length * RFS_SECTOR_SIZE;
	if (length > reserved_bytes) {
		dprintf("rfs: truncate_file: requested %lu > reserved %lu\n", length, reserved_bytes);
		return false;
	}

	/* No-op if unchanged. */
	if (length == ent.length) {
		return true;
	}

	/* Update on-disk size; keep sector_length as-is. */
	fs_directory_entry_t de = (fs_directory_entry_t){0};
	de.directory = tree;
	de.filename  = file->filename;
	de.lbapos    = file->lbapos;          /* unchanged */
	de.flags     = 0;                     /* file */
	de.size      = length;      /* new logical size */

	if (!rfs_upsert_directory_entry(&de, ent.sector_length)) {
		dprintf("rfs: truncate_file: failed to update directory entry\n");
		return false;
	}

	/* Reflect in VFS node. */
	file->size = length;
	return true;
}

