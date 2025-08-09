#include <kernel.h>
#include <retrofs.h>

uint64_t rfs_create_directory(void *dir, const char *name) {
	if (!dir || !name || !*name) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}

	fs_tree_t *tree = (fs_tree_t *) dir;
	rfs_t *info = (rfs_t *) tree->opaque;
	if (!info || !info->desc) {
		fs_set_error(FS_ERR_VFS_DATA);
		return 0;
	}

	/* Reject duplicate (case-insensitive) */
	{
		uint64_t s = 0;
		size_t i = 0;
		rfs_directory_entry_inner_t e;
		if (rfs_locate_entry(info, tree, name, &s, &i, &e)) {
			fs_set_error(FS_ERR_DIRECTORY_EXISTS);
			return 0;
		}
	}

	/* Parent start sector (root if 0) */
	uint64_t parent_start = tree->lbapos ? tree->lbapos : info->desc->root_directory;

	/* Allocate a directory block (fixed size) */
	const uint64_t dir_sectors = RFS_DEFAULT_DIR_SIZE;
	uint64_t start_sector = 0;

	/* Prepare zero buffer before marking (avoids rollback on OOM) */
	const uint64_t chunk_sectors = RFS_MAP_READ_CHUNK_SECTORS; /* 128 sectors */
	const size_t chunk_bytes = chunk_sectors * RFS_SECTOR_SIZE;
	unsigned char *zero = (unsigned char *) kmalloc(chunk_bytes);
	if (!zero) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	memset(zero, 0, chunk_bytes);

	/* FIXME(locks): racy between find_free_extent() and mark_extent(). */
	if (!rfs_find_free_extent(info, dir_sectors, &start_sector)) {
		fs_set_error(FS_ERR_NO_SPACE);
		kfree(zero);
		return 0;
	}
	if (!rfs_mark_extent(info, start_sector, dir_sectors, true)) {
		kfree(zero);
		return 0;
	}

	/* Zero the new directory block */
	{
		uint64_t remaining = dir_sectors;
		uint64_t pos = start_sector;
		while (remaining) {
			uint64_t this_secs = (remaining > chunk_sectors) ? chunk_sectors : remaining;
			size_t this_bytes = this_secs * RFS_SECTOR_SIZE;
			if (!rfs_write_device(info, pos, this_bytes, zero)) {
				kfree(zero);
				rfs_mark_extent(info, start_sector, dir_sectors, false);
				return 0;
			}
			pos += this_secs;
			remaining -= this_secs;
		}
	}
	kfree(zero);

	/* Write the directory start entry */
	{
		rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];
		memset(block, 0, sizeof(block));

		rfs_directory_start_t *start = (rfs_directory_start_t *) &block[0];
		start->flags = RFS_FLAG_DIR_START;
		strlcpy(start->title, name, RFS_MAX_NAME);
		start->parent = parent_start;
		start->sectors = RFS_DEFAULT_DIR_SIZE;
		start->continuation = 0;

		if (!rfs_write_device(info, start_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			rfs_mark_extent(info, start_sector, dir_sectors, false);
			return 0;
		}
	}

	/* Insert entry into parent directory */
	{
		fs_directory_entry_t de = (fs_directory_entry_t) {0};
		de.directory = tree;
		de.filename = (char *) name;
		de.lbapos = start_sector;
		de.flags = FS_DIRECTORY;
		de.size = 0;

		if (!rfs_upsert_directory_entry(&de, RFS_DEFAULT_DIR_SIZE)) {
			rfs_mark_extent(info, start_sector, dir_sectors, false);
			return 0;
		}
	}

	/* Return on-disk address of new directory */
	return start_sector;
}
