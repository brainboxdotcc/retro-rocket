#include <kernel.h>
#include <retrofs.h>

/* Extend-and-move: allocate a new extent big enough for min_bytes, zero it,
 * copy old logical bytes across, free old extent, update dir entry & VFS node. */
static bool rfs_extend_and_move(fs_tree_t* tree, rfs_t* info, fs_directory_entry_t* file, uint64_t min_bytes_required) {
	/* Locate current on-disk entry */
	uint64_t blk_sector = 0;
	size_t   blk_index  = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, file->filename, &blk_sector, &blk_index, &ent)) {
		dprintf("rfs: extend_move: locate failed for '%s'\n", file->filename);
		return false;
	}
	if ((ent.flags & RFS_FLAG_DIRECTORY) != 0) {
		dprintf("rfs: extend_move: '%s' is a directory\n", file->filename);
		return false;
	}

	const uint64_t old_start   = ent.sector_start;
	const uint64_t old_sectors = ent.sector_length;
	const uint64_t old_bytes   = ent.length;

	const uint64_t new_sectors = rfs_bytes_to_sectors(min_bytes_required);
	uint64_t       new_start   = 0;

	/* Buffers: one zero buffer for zero-fill, one bulk buffer for copying */
	const uint64_t chunk_sectors = RFS_MAP_READ_CHUNK_SECTORS;
	const size_t   chunk_bytes   = (size_t)(chunk_sectors * RFS_SECTOR_SIZE);

	unsigned char* zero = (unsigned char*)kmalloc(chunk_bytes);
	if (!zero) {
		dprintf("rfs: extend_move: OOM zero buffer\n");
		return false;
	}
	memset(zero, 0, chunk_bytes);

	unsigned char* bulk = (unsigned char*)kmalloc(chunk_bytes);
	if (!bulk) {
		dprintf("rfs: extend_move: OOM bulk buffer\n");
		kfree(zero);
		return false;
	}

	/* FIXME(locks): racy between find_free_extent() and mark_extent(). */
	if (!rfs_find_free_extent(info, new_sectors, &new_start)) {
		dprintf("rfs: extend_move: no space for %llu sectors\n", (unsigned long long)new_sectors);
		kfree(bulk);
		kfree(zero);
		return false;
	}
	if (!rfs_mark_extent(info, new_start, new_sectors, true)) {
		dprintf("rfs: extend_move: mark new extent failed\n");
		kfree(bulk);
		kfree(zero);
		return false;
	}

	/* Zero-fill the new extent */
	{
		uint64_t remaining = new_sectors, pos = new_start;
		while (remaining) {
			const uint64_t this_secs  = (remaining > chunk_sectors) ? chunk_sectors : remaining;
			const size_t   this_bytes = (size_t)(this_secs * RFS_SECTOR_SIZE);
			if (!rfs_write_device(info, pos, this_bytes, zero)) {
				dprintf("rfs: extend_move: zeroing failed at %llu\n", (unsigned long long)pos);
				/* Leave marked; better a space leak than dangling refs. */
				kfree(bulk);
				kfree(zero);
				return false;
			}
			pos       += this_secs;
			remaining -= this_secs;
		}
	}

	/* Copy existing logical bytes old -> new */
	if (old_bytes != 0 && old_sectors != 0) {
		uint64_t bytes_left = old_bytes;

		/* Bulk full sectors first */
		const uint64_t full_secs_total = bytes_left / RFS_SECTOR_SIZE;
		uint64_t       src_sector      = old_start;
		uint64_t       dst_sector      = new_start;
		uint64_t       secs_remaining  = full_secs_total;

		while (secs_remaining) {
			const uint64_t this_secs  = (secs_remaining > chunk_sectors) ? chunk_sectors : secs_remaining;
			const size_t   this_bytes = (size_t)(this_secs * RFS_SECTOR_SIZE);

			if (!rfs_read_device(info, src_sector, this_bytes, bulk)) {
				dprintf("rfs: extend_move: bulk read failed at %llu\n", (unsigned long long)src_sector);
				kfree(bulk);
				kfree(zero);
				return false;
			}
			if (!rfs_write_device(info, dst_sector, this_bytes, bulk)) {
				dprintf("rfs: extend_move: bulk write failed at %llu\n", (unsigned long long)dst_sector);
				kfree(bulk);
				kfree(zero);
				return false;
			}

			src_sector    += this_secs;
			dst_sector    += this_secs;
			secs_remaining -= this_secs;
		}

		/* Tail partial sector (if any) */
		const size_t tail_bytes = (size_t)(bytes_left % RFS_SECTOR_SIZE);
		if (tail_bytes) {
			unsigned char src_sec[RFS_SECTOR_SIZE];
			unsigned char dst_sec[RFS_SECTOR_SIZE];
			memset(dst_sec, 0, sizeof(dst_sec));

			if (!rfs_read_device(info, src_sector, RFS_SECTOR_SIZE, src_sec)) {
				dprintf("rfs: extend_move: read tail failed at %llu\n", (unsigned long long)src_sector);
				kfree(bulk);
				kfree(zero);
				return false;
			}
			memcpy(dst_sec, src_sec, tail_bytes);
			if (!rfs_write_device(info, dst_sector, RFS_SECTOR_SIZE, dst_sec)) {
				dprintf("rfs: extend_move: write tail failed at %llu\n", (unsigned long long)dst_sector);
				kfree(bulk);
				kfree(zero);
				return false;
			}
		}
	}

	kfree(bulk);
	kfree(zero);

	/* Free the old extent (best-effort) */
	if (old_sectors > 0) {
		if (!rfs_mark_extent(info, old_start, old_sectors, false)) {
			dprintf("rfs: extend_move: WARNING: failed to free old extent (space leak)\n");
		}
	}

	/* Update dir entry on disk (new start + sector_length, preserve size) */
	{
		fs_directory_entry_t de = (fs_directory_entry_t){0};
		de.directory = tree;
		de.filename  = file->filename;
		de.lbapos    = new_start;
		de.flags     = 0;           /* file */
		de.size      = old_bytes;   /* logical bytes unchanged by move */
		if (!rfs_upsert_directory_entry(&de, new_sectors)) {
			dprintf("rfs: extend_move: upsert failed after move (metadata mismatch risk)\n");
			/* We keep the new extent as best-effort. */
		}
	}

	/* Update VFS node */
	file->lbapos = new_start;

	return true;
}

bool rfs_write_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer) {
	if (!f || (!length)) {
		return (length == 0); /* zero-length write is a no-op success */
	}
	if (!buffer) {
		dprintf("rfs: write_file: null buffer\n");
		return false;
	}

	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	if (!tree) {
		dprintf("rfs: write_file: missing VFS tree\n");
		return false;
	}
	rfs_t* info = (rfs_t*)tree->opaque;
	if (!info || !info->desc) {
		dprintf("rfs: write_file: missing fs context\n");
		return false;
	}

	/* Get on-disk capacity (sector_length) and sanity-check it's a file */
	uint64_t blk_sector = 0;
	size_t   blk_index  = 0;
	rfs_directory_entry_inner_t ent;
	if (!rfs_locate_entry(info, tree, file->filename, &blk_sector, &blk_index, &ent)) {
		dprintf("rfs: write_file: locate failed for '%s'\n", file->filename);
		return false;
	}
	if ((ent.flags & RFS_FLAG_DIRECTORY) != 0) {
		dprintf("rfs: write_file: '%s' is a directory\n", file->filename);
		return false;
	}

	/* End position of the write in bytes */
	uint64_t end_pos = start + (uint64_t)length;

	/* Ensure capacity: move if we exceed reserved bytes */
	uint64_t reserved_bytes = ent.sector_length * (uint64_t)RFS_SECTOR_SIZE;
	if (end_pos > reserved_bytes) {
		/* Extend + move to at least end_pos bytes */
		if (!rfs_extend_and_move(tree, info, file, end_pos)) {
			return false;
		}
		/* Refresh metadata */
		if (!rfs_locate_entry(info, tree, file->filename, &blk_sector, &blk_index, &ent)) {
			dprintf("rfs: write_file: locate-after-extend failed\n");
			return false;
		}
	}

	/* Now perform the write with RMW at edges */
	uint64_t file_start_sector = file->lbapos;
	uint64_t first_sector = file_start_sector + (start / RFS_SECTOR_SIZE);
	uint64_t last_sector  = file_start_sector + ((end_pos - 1) / RFS_SECTOR_SIZE);
	size_t   head_off     = (size_t)(start % RFS_SECTOR_SIZE);

	unsigned char* sector_buf = (unsigned char*)kmalloc(RFS_SECTOR_SIZE);
	if (!sector_buf) {
		dprintf("rfs: write_file: OOM sector buf\n");
		return false;
	}

	/* Single-sector write */
	if (first_sector == last_sector) {
		if (!rfs_read_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: write_file: read head/tail sector failed\n");
			kfree(sector_buf);
			return false;
		}
		memcpy(sector_buf + head_off, buffer, length);
		if (!rfs_write_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: write_file: write head/tail sector failed\n");
			kfree(sector_buf);
			return false;
		}
		kfree(sector_buf);
	} else {
		/* Head partial */
		size_t head_bytes = RFS_SECTOR_SIZE - head_off;
		if (!rfs_read_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: write_file: read head failed\n");
			kfree(sector_buf);
			return false;
		}
		memcpy(sector_buf + head_off, buffer, head_bytes);
		if (!rfs_write_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: write_file: write head failed\n");
			kfree(sector_buf);
			return false;
		}

		/* Middle full sectors */
		uint64_t cur_sector = first_sector + 1;
		uint64_t full_bytes_remaining = (uint64_t)length - head_bytes;
		uint64_t full_sectors = (full_bytes_remaining / RFS_SECTOR_SIZE);
		const uint64_t chunk_sectors = RFS_MAP_READ_CHUNK_SECTORS;

		const unsigned char* cur_buf = buffer + head_bytes;

		while (full_sectors) {
			uint64_t this_secs  = (full_sectors > chunk_sectors) ? chunk_sectors : full_sectors;
			size_t   this_bytes = (size_t)(this_secs * RFS_SECTOR_SIZE);
			if (!rfs_write_device(info, cur_sector, this_bytes, cur_buf)) {
				dprintf("rfs: write_file: write middle failed at %llu\n", (unsigned long long)cur_sector);
				kfree(sector_buf);
				return false;
			}
			cur_sector += this_secs;
			cur_buf    += this_bytes;
			full_sectors -= this_secs;
		}

		/* Tail partial */
		size_t tail_bytes = (size_t)((start + (uint64_t)length) % RFS_SECTOR_SIZE);
		if (tail_bytes == 0) {
			tail_bytes = RFS_SECTOR_SIZE;
		}
		/* tail_bytes currently equals bytes that land in the last sector;
		   but if the write ends exactly on a sector boundary, the above sets
		   to full sector; we only need to RMW when there's a partial tail. */
		if (((start + (uint64_t)length) % RFS_SECTOR_SIZE) != 0) {
			if (!rfs_read_device(info, last_sector, RFS_SECTOR_SIZE, sector_buf)) {
				dprintf("rfs: write_file: read tail failed\n");
				kfree(sector_buf);
				return false;
			}
			size_t remaining_tail = (size_t)((start + (uint64_t)length) % RFS_SECTOR_SIZE);
			memcpy(sector_buf, cur_buf, remaining_tail);
			if (!rfs_write_device(info, last_sector, RFS_SECTOR_SIZE, sector_buf)) {
				dprintf("rfs: write_file: write tail failed\n");
				kfree(sector_buf);
				return false;
			}
		}
		kfree(sector_buf);
	}

	/* Update logical size if extended (preserve sector_length) */
	if (end_pos > ent.length) {
		fs_directory_entry_t de = {0};
		de.directory = tree;
		de.filename  = file->filename;
		de.lbapos    = file->lbapos;     /* may have changed if we moved */
		de.flags     = 0;                /* file */
		de.size      = end_pos;          /* new logical size */

		if (!rfs_upsert_directory_entry(&de, 0)) { /* 0 => keep sector_length unchanged */
			dprintf("rfs: write_file: warning: size update failed\n");
			/* Not fatal for the write itself; data is on disk. */
		}

		file->size = end_pos; /* reflect in VFS node */
	}

	return true;
}
