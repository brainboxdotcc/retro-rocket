#include <kernel.h>
#include <retrofs.h>

#include <kernel.h>
#include <retrofs.h>

bool rfs_read_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer) {
	if (!f) {
		dprintf("rfs: read_file: null file\n");
		return false;
	}
	if (length == 0) {
		return true; /* no-op */
	}
	if (!buffer) {
		dprintf("rfs: read_file: null buffer\n");
		return false;
	}

	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	if (file->flags & FS_DIRECTORY) {
		dprintf("rfs: read_file: '%s' is a directory\n", file->filename ? file->filename : "(unnamed)");
		return false;
	}

	fs_tree_t* tree = (fs_tree_t*)file->directory;
	if (!tree) {
		dprintf("rfs: read_file: missing VFS tree\n");
		return false;
	}
	rfs_t* info = (rfs_t*)tree->opaque;
	if (!info || !info->desc) {
		dprintf("rfs: read_file: missing fs context\n");
		return false;
	}

	/* Bounds check against logical size */
	uint64_t size_bytes = file->size;
	if (start >= size_bytes) {
		dprintf("rfs: read_file: start beyond EOF (start=%llu size=%llu)\n",
			(unsigned long long)start, (unsigned long long)size_bytes);
		return false;
	}
	if (start + (uint64_t)length > size_bytes) {
		dprintf("rfs: read_file: range beyond EOF (start=%llu len=%u size=%llu)\n",
			(unsigned long long)start, length, (unsigned long long)size_bytes);
		return false;
	}

	/* Work out sectors to touch */
	uint64_t file_start_sector = file->lbapos; /* start of file's extent */
	uint64_t end_pos           = start + (uint64_t)length;

	uint64_t first_sector = file_start_sector + (start        / RFS_SECTOR_SIZE);
	uint64_t last_sector  = file_start_sector + ((end_pos - 1) / RFS_SECTOR_SIZE);
	size_t   head_off     = (size_t)(start % RFS_SECTOR_SIZE);

	/* Single-sector read (entire request within one sector) */
	if (first_sector == last_sector) {
		unsigned char* sector_buf = (unsigned char*)kmalloc(RFS_SECTOR_SIZE);
		if (!sector_buf) {
			dprintf("rfs: read_file: OOM sector buffer\n");
			return false;
		}
		if (!rfs_read_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: read_file: read failed at sector %llu\n", (unsigned long long)first_sector);
			kfree(sector_buf);
			return false;
		}
		memcpy(buffer, sector_buf + head_off, length);
		kfree(sector_buf);
		return true;
	}

	/* Multi-sector read: head partial, middle full, tail partial */
	unsigned char* sector_buf = (unsigned char*)kmalloc(RFS_SECTOR_SIZE);
	if (!sector_buf) {
		dprintf("rfs: read_file: OOM sector buffer\n");
		return false;
	}

	/* Head partial */
	size_t head_bytes = RFS_SECTOR_SIZE - head_off;
	if (!rfs_read_device(info, first_sector, RFS_SECTOR_SIZE, sector_buf)) {
		dprintf("rfs: read_file: head read failed at %llu\n", (unsigned long long)first_sector);
		kfree(sector_buf);
		return false;
	}
	memcpy(buffer, sector_buf + head_off, head_bytes);

	/* Middle full sectors: read directly into the user's buffer slice */
	uint64_t cur_sector = first_sector + 1;
	uint64_t bytes_done = head_bytes;
	uint64_t full_bytes_remaining = (uint64_t)length - head_bytes;
	uint64_t full_sectors = full_bytes_remaining / RFS_SECTOR_SIZE;

	while (full_sectors) {
		uint64_t batch_secs  = full_sectors;
		if (batch_secs > RFS_MAP_READ_CHUNK_SECTORS) {
			batch_secs = RFS_MAP_READ_CHUNK_SECTORS;
		}
		size_t batch_bytes = (size_t)(batch_secs * RFS_SECTOR_SIZE);

		if (!rfs_read_device(info, cur_sector, batch_bytes, buffer + bytes_done)) {
			dprintf("rfs: read_file: middle read failed at %llu\n", (unsigned long long)cur_sector);
			kfree(sector_buf);
			return false;
		}

		cur_sector += batch_secs;
		bytes_done += batch_bytes;
		full_sectors -= batch_secs;
	}

	/* Tail partial (only if not sector-aligned end) */
	size_t tail_bytes = (size_t)((start + (uint64_t)length) % RFS_SECTOR_SIZE);
	if (tail_bytes != 0) {
		if (!rfs_read_device(info, last_sector, RFS_SECTOR_SIZE, sector_buf)) {
			dprintf("rfs: read_file: tail read failed at %llu\n", (unsigned long long)last_sector);
			kfree(sector_buf);
			return false;
		}
		memcpy(buffer + bytes_done, sector_buf, tail_bytes);
	}

	kfree(sector_buf);
	return true;
}
