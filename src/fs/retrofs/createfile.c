#include <kernel.h>
#include <retrofs.h>

/* Table of default size reservations by extension (MB) */
typedef struct {
	const char *ext;
	uint32_t size_kb;
} rfs_ext_size_t;

static const rfs_ext_size_t rfs_ext_size_table[] = {
	{"jpeg", 4096},
	{"jpg",  4096},
	{"png",  4096},
	{"gif",  4096},
	{"bmp",  4096},
	{"webp", 4096},
	{"tif",  4096},
	{"tiff", 4096},
};

fs_directory_entry_t *rfs_upsert_directory_entry(fs_directory_entry_t *file, uint64_t sector_extent) {
	if (file == NULL || file->directory == NULL) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	fs_tree_t *tree = (fs_tree_t *) file->directory;
	rfs_t *info = (rfs_t *) tree->opaque;
	if (info == NULL || info->desc == NULL) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	const char *target_name = file->filename;
	if (target_name == NULL || target_name[0] == '\0') {
		fs_set_error(FS_ERR_INVALID_FILENAME);
		return NULL;
	}

	/* Directory chain scan state */
	uint64_t current_sector = (tree->lbapos != 0) ? tree->lbapos : info->desc->root_directory;

	/* Keep a remembered first-empty slot across the chain */
	bool have_empty_slot = false;
	uint64_t empty_slot_sector = 0;
	size_t empty_slot_index = 0;

	/* One directory block buffer (64 sectors -> 128 half-sector entries) */
	rfs_directory_entry_t block[RFS_DEFAULT_DIR_SIZE * 2];

	/* Walk limit for corruption protection */
	uint32_t walked = 0;
	const uint32_t walk_limit = 1u << 16;

	while (current_sector != 0) {
		if (walked++ > walk_limit) {
			dprintf("upsert past walk limit\n");
			fs_set_error(FS_ERR_CYCLE_DETECTED);
			return NULL;
		}

		if (!rfs_read_device(info, current_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("rfs_read_device failed in upsert\n");
			return NULL;
		}

		rfs_directory_start_t *start = (rfs_directory_start_t *) &block[0];
		if ((start->flags & RFS_FLAG_DIR_START) == 0 || start->sectors != RFS_DEFAULT_DIR_SIZE) {
			fs_set_error(FS_ERR_BROKEN_DIRECTORY);
			return NULL;
		}

		rfs_directory_entry_t *ents = (rfs_directory_entry_t *) &block[1];
		const size_t entries_per_block = (size_t) ((RFS_DEFAULT_DIR_SIZE * 2) - 1);

		/* First pass: try to find an existing entry (case-insensitive) */
		for (size_t i = 0; i < entries_per_block; i++) {
			rfs_directory_entry_inner_t *e = &ents[i].entry;

			if (e->filename[0] == '\0') {
				/* Remember first empty slot in the chain */
				if (!have_empty_slot) {
					have_empty_slot = true;
					empty_slot_sector = current_sector;
					empty_slot_index = i;
				}
				break; /* end of this block's list */
			}

			if (strcasecmp(e->filename, target_name) == 0) {
				/* Update existing entry; preserve case from file->filename */
				memset(e->filename, 0, RFS_MAX_NAME);
				strlcpy(e->filename, file->filename, RFS_MAX_NAME);
				e->sector_start = file->lbapos;
				e->length = file->size;
				e->sector_length = sector_extent;
				e->flags &= ~RFS_FLAG_DIRECTORY;
				if ((file->flags & FS_DIRECTORY) != 0) {
					e->flags |= RFS_FLAG_DIRECTORY;
				}
				/* Caller may handle timestamps/sequence; we avoid policy here */

				if (!rfs_write_device(info, current_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
					dprintf("upsert write failed\n");
					return NULL;
				}

				return file;
			}
		}

		if (start->continuation == 0) {
			break;
		}

		current_sector = start->continuation;
	}

	/* No existing entry found: insert at the first available empty slot if any */
	if (have_empty_slot) {
		if (!rfs_read_device(info, empty_slot_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("upsert read(2) fail\n");
			return NULL;
		}

		rfs_directory_entry_t *ents = (rfs_directory_entry_t *) &block[1];
		rfs_directory_entry_inner_t *e = &ents[empty_slot_index].entry;

		/* Write new entry; preserve case from file->filename */
		strlcpy(e->filename, file->filename, RFS_MAX_NAME);
		e->sector_start = file->lbapos;
		e->length = file->size;
		e->sector_length = sector_extent;
		e->flags = 0;
		if ((file->flags & FS_DIRECTORY) != 0) {
			e->flags |= RFS_FLAG_DIRECTORY;
		}
		/* created/modified left to higher layer policy */

		if (!rfs_write_device(info, empty_slot_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("upsert write(2) fail\n");
			return NULL;
		}

		return file;
	}

	/* Directory is full: extend with a new block and insert there. This needs FS map updates for the directory block itself. */
	{
		uint64_t new_block_sector = 0;
		if (!rfs_find_free_extent(info, RFS_DEFAULT_DIR_SIZE, &new_block_sector)) {
			dprintf("rfs: upsert: no space for new dir block\n");
			return NULL;
		}
		if (!rfs_mark_extent(info, new_block_sector, RFS_DEFAULT_DIR_SIZE, true)) {
			dprintf("rfs: upsert: failed to mark new dir block used\n");
			return NULL;
		}

		/* Read the last block again so we can patch its continuation */
		uint64_t last_sector = (tree->lbapos != 0) ? tree->lbapos : info->desc->root_directory;
		uint64_t next = last_sector;
		while (true) {
			if (!rfs_read_device(info, next, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
				dprintf("upsert read(3) fail\n");
				return NULL;
			}
			rfs_directory_start_t *s = (rfs_directory_start_t *) &block[0];
			if (s->continuation == 0) {
				last_sector = next;
				break;
			}
			next = s->continuation;
		}

		/* Patch tail's continuation to point at new block */
		rfs_directory_start_t *tail = (rfs_directory_start_t *) &block[0];
		tail->continuation = new_block_sector;
		if (!rfs_write_device(info, last_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &block[0])) {
			dprintf("upsert write(3) fail\n");
			return NULL;
		}

		/* Build a fresh directory block at new_block_sector */
		rfs_directory_entry_t newblk[RFS_DEFAULT_DIR_SIZE * 2];
		memset(&newblk[0], 0, sizeof(newblk));

		rfs_directory_start_t *nstart = (rfs_directory_start_t *) &newblk[0];
		nstart->flags = RFS_FLAG_DIR_START;            /* start of a directory block */
		nstart->parent = ((fs_tree_t *) file->directory)->lbapos ? ((fs_tree_t *) file->directory)->lbapos
									 : info->desc->root_directory;
		nstart->sectors = RFS_DEFAULT_DIR_SIZE;
		nstart->continuation = 0;

		rfs_directory_entry_inner_t *nents = (rfs_directory_entry_inner_t *) &newblk[1];
		rfs_directory_entry_inner_t *ne = &nents[0];

		strlcpy(ne->filename, file->filename, RFS_MAX_NAME);
		ne->sector_start = file->lbapos;
		ne->length = file->size;
		ne->sector_length = sector_extent;
		ne->flags = 0;
		if ((file->flags & FS_DIRECTORY) != 0) {
			ne->flags |= RFS_FLAG_DIRECTORY;
		}

		if (!rfs_write_device(info, new_block_sector, RFS_SECTOR_SIZE * RFS_DEFAULT_DIR_SIZE, &newblk[0])) {
			dprintf("upsert write(4) fail\n");
			return NULL;
		}

		return file;
	}
}

size_t rfs_get_default_reservation(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if (dot && *(dot + 1)) {
		const char *ext = dot + 1;
		for (size_t i = 0; i < sizeof(rfs_ext_size_table) / sizeof(rfs_ext_size_table[0]); i++) {
			if (strcasecmp(ext, rfs_ext_size_table[i].ext) == 0) {
				return (size_t) rfs_ext_size_table[i].size_kb * 1024;
			}
		}
	}
	return 128 * 1024; /* Default: 128 KB */
}

uint64_t rfs_bytes_to_sectors(uint64_t bytes) {
	if (bytes == 0) {
		return 0;
	}
	return (bytes + (RFS_SECTOR_SIZE - 1)) / RFS_SECTOR_SIZE;
}

uint64_t rfs_create_file(void *dir, const char *name, size_t size) {
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

	/* 1) Don’t clobber an existing entry (case-insensitive per RFS rules) */
	{
		uint64_t s = 0;
		size_t i = 0;
		rfs_directory_entry_inner_t e;
		if (rfs_locate_entry(info, tree, name, &s, &i, &e)) {
			fs_set_error(FS_ERR_VFS_DATA);
			return 0;
		}
	}

	/* 2) Work out reservation: default table (MB) vs requested size */
	size_t reserve_bytes = rfs_get_default_reservation(name);
	if (size > reserve_bytes) {
		reserve_bytes = size;
	}
	uint64_t reserve_sectors = (reserve_bytes + (RFS_SECTOR_SIZE - 1)) / RFS_SECTOR_SIZE;
	if (reserve_sectors == 0) {
		reserve_sectors = 1; /* keep it simple: at least one sector */
	}

	/* 3) Allocate a contiguous extent */
	uint64_t start_sector = 0;
	if (!rfs_find_free_extent(info, reserve_sectors, &start_sector)) {
		fs_set_error(FS_ERR_NO_SPACE);
		return 0;
	}
	dprintf("Got reservation at sector %lu\n", start_sector);
	if (start_sector < 65) {
		dprintf("Invalid reservation (would trample root)\n");
		return 0;
	}
	/* 4) Zero the allocated extent (security) */
	const uint64_t chunk_sectors = RFS_MAP_READ_CHUNK_SECTORS; /* 128 sectors = 64 KiB */
	const size_t chunk_bytes = (size_t) (chunk_sectors * RFS_SECTOR_SIZE);
	unsigned char *zero = (unsigned char *) kmalloc(chunk_bytes);
	if (!zero) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	memset(zero, 0, chunk_bytes);

	if (!rfs_mark_extent(info, start_sector, reserve_sectors, true)) {
		kfree_null(&zero);
		return 0;
	}

	uint64_t remaining = reserve_sectors, pos = start_sector;
	while (remaining) {
		uint64_t this_sectors = (remaining > chunk_sectors) ? chunk_sectors : remaining;
		uint64_t this_bytes = this_sectors * RFS_SECTOR_SIZE;
		if (!rfs_write_device(info, pos, this_bytes, zero)) {
			kfree_null(&zero);
			rfs_mark_extent(info, start_sector, reserve_sectors, false);
			return 0;
		}
		pos += this_sectors;
		remaining -= this_sectors;
	}
	kfree_null(&zero);

	/* 5) Upsert the directory entry (don’t duplicate that logic) */
	fs_directory_entry_t de = {0};
	de.directory = tree;         /* parent directory */
	de.filename = (char *) name;  /* preserve case as given */
	de.lbapos = start_sector; /* start of data */
	de.flags = 0;            /* file, not directory */
	de.size = (uint64_t) size;

	if (!rfs_upsert_directory_entry(&de, reserve_sectors)) {
		dprintf("rfs: create_file: upsert failed, rolling back extent\n");
		rfs_mark_extent(info, start_sector, reserve_sectors, false);
		return 0;
	}

	/* VFS contract: return on-disk address */
	return start_sector;
}
