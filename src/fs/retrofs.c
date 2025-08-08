#include <kernel.h>

filesystem_t* rfs_fs = NULL;

int rfs_read_device(rfs_t* rfs, uint64_t start_sectors, uint64_t size_bytes, void* buffer)
{
	const uint64_t total_sectors = rfs->length / RFS_SECTOR_SIZE;
	const uint64_t nsectors = size_bytes / RFS_SECTOR_SIZE;

	if ((size_bytes % RFS_SECTOR_SIZE) != 0) {
		dprintf("rfs_read_device: size not sector-aligned\n");
		return 0;
	}
	if (start_sectors + nsectors > total_sectors) {
		dprintf("rfs_read_device: would read past end of device\n");
		return 0;
	}

	uint64_t cluster_start = rfs->start / RFS_SECTOR_SIZE;
	return read_storage_device(rfs->dev->name, cluster_start + start_sectors, size_bytes, buffer);
}

int rfs_write_device(rfs_t* rfs, uint64_t start_sectors, uint64_t size_bytes, void* buffer)
{
	const uint64_t total_sectors = rfs->length / RFS_SECTOR_SIZE;
	const uint64_t nsectors = size_bytes / RFS_SECTOR_SIZE;

	if ((size_bytes % RFS_SECTOR_SIZE) != 0) {
		dprintf("rfs_write_device: size not sector-aligned\n");
		return 0;
	}
	if (start_sectors + nsectors > total_sectors) {
		dprintf("rfs_write_device: would read past end of device\n");
		return 0;
	}

	uint64_t cluster_start = rfs->start / RFS_SECTOR_SIZE;
	dprintf("rfs_write_device(%lu,%lu)\n", cluster_start + start_sectors, size_bytes / RFS_SECTOR_SIZE);
	return write_storage_device(rfs->dev->name, cluster_start + start_sectors, size_bytes, buffer);
}

bool rfs_reserve_in_fs_map(rfs_t* info, uint64_t start, uint64_t length)
{
	if (!info || !info->desc) {
		return false;
	}

	uint64_t map_start_sector = info->desc->free_space_map_start;
	uint64_t map_length_sectors = info->desc->free_space_map_length;
	uint64_t total_bits = (map_length_sectors * RFS_SECTOR_SIZE) * 8ULL;

	if (start + length > total_bits) {
		dprintf("rfs_reserve_in_fs_map: out of range (start=%lu len=%lu total=%lu)\n",
			start, length, total_bits);
		return false;
	}

	// We may touch multiple map sectors
	uint64_t bit_index = start;
	uint64_t end_bit   = start + length;

	while (bit_index < end_bit) {
		// Which sector of the map holds this bit?
		uint64_t sector_offset = (bit_index / (RFS_SECTOR_SIZE * 8ULL));
		uint64_t sector_lba = map_start_sector + sector_offset;

		// Load that sector of the map
		uint8_t buf[RFS_SECTOR_SIZE];
		if (!rfs_read_device(info, sector_lba, RFS_SECTOR_SIZE, buf)) {
			dprintf("rfs_reserve_in_fs_map: failed to read map sector %lu\n", sector_lba);
			return false;
		}

		// Modify bits within this sector
		uint64_t first_bit_in_sector = sector_offset * RFS_SECTOR_SIZE * 8ULL;
		uint64_t last_bit_in_sector  = first_bit_in_sector + (RFS_SECTOR_SIZE * 8ULL);

		uint64_t local_start = bit_index - first_bit_in_sector;
		uint64_t local_end   = (end_bit > last_bit_in_sector)
				       ? (RFS_SECTOR_SIZE * 8ULL)
				       : (end_bit - first_bit_in_sector);

		for (uint64_t b = local_start; b < local_end; b++) {
			buf[b / 8] |= (1 << (b % 8));
		}

		// Write it back
		if (!rfs_write_device(info, sector_lba, RFS_SECTOR_SIZE, buf)) {
			dprintf("rfs_reserve_in_fs_map: failed to write map sector %lu\n", sector_lba);
			return false;
		}

		// Advance to next sector if needed
		bit_index = first_bit_in_sector + local_end;
	}

	return true;
}

static bool rfs_build_level_caches(rfs_t *info)
{
	if (!info || !info->desc) return false;

	// Geometry
	info->total_sectors = info->length / RFS_SECTOR_SIZE;

	const uint64_t G = RFS_L1_GROUP_SECTORS;       // sectors per L1 group
	const uint64_t S = RFS_L2_GROUPS_PER_SUPER;    // L1 groups per L2 super-group

	info->l1_groups = (info->total_sectors + G - 1ULL) / G;
	info->l2_groups = (info->l1_groups   + S - 1ULL) / S;

	// Allocate RAM structures
	info->l1_free_count = kmalloc(info->l1_groups * sizeof(uint16_t));
	if (!info->l1_free_count) return false;
	memset(info->l1_free_count, 0, info->l1_groups * sizeof(uint16_t));

	const size_t l1_bytes = bitset_bytes(info->l1_groups);
	info->l1_not_full = kmalloc(l1_bytes);
	info->l1_all_free = kmalloc(l1_bytes);
	if (!info->l1_not_full || !info->l1_all_free) {
		kfree_null(&info->l1_not_full);
		kfree_null(&info->l1_all_free);
		kfree_null(&info->l1_free_count);
		return false;
	}
	memset(info->l1_not_full, 0, l1_bytes);
	memset(info->l1_all_free, 0, l1_bytes);

	const size_t l2_bytes = bitset_bytes(info->l2_groups);
	info->l2_not_full = kmalloc(l2_bytes);
	info->l2_all_free = kmalloc(l2_bytes);
	if (!info->l2_not_full || !info->l2_all_free) {
		kfree_null(&info->l2_not_full);
		kfree_null(&info->l2_all_free);
		kfree_null(&info->l1_not_full);
		kfree_null(&info->l1_all_free);
		kfree_null(&info->l1_free_count);
		return false;
	}
	memset(info->l2_not_full, 0, l2_bytes);
	memset(info->l2_all_free, 0, l2_bytes);

	// Stream the Level-0 bitmap (on disk: 1 bit per sector, 1=used, 0=free)
	const uint64_t map_start = info->desc->free_space_map_start;
	const uint64_t map_len   = info->desc->free_space_map_length;
	const uint64_t total_bits = info->total_sectors;

	const uint64_t max_bytes_per_read = RFS_MAP_READ_CHUNK_SECTORS * RFS_SECTOR_SIZE;
	uint8_t *buf = kmalloc(max_bytes_per_read);
	if (!buf) {
		dprintf("rfs_build_level_caches: OOM map buffer\n");
		kfree_null(&info->l2_not_full);
		kfree_null(&info->l2_all_free);
		kfree_null(&info->l1_not_full);
		kfree_null(&info->l1_all_free);
		kfree_null(&info->l1_free_count);
		return false;
	}

	uint64_t bit_cursor = 0;   // global sector-index represented by next L0 bit
	uint64_t sector_off = 0;   // offset into L0 map (in sectors)

	while (sector_off < map_len && bit_cursor < total_bits) {
		const uint64_t remaining      = map_len - sector_off;
		const uint64_t sectors_this   = MIN(remaining, RFS_MAP_READ_CHUNK_SECTORS);
		if (sectors_this == 0) {
			break; // nothing left
		}
		const size_t bytes_this = (size_t)(sectors_this * RFS_SECTOR_SIZE);
		if (!rfs_read_device(info, map_start + sector_off, bytes_this, buf)) {
			dprintf("rfs_build_level_caches: failed read @LBA %lu (sectors=%lu)\n",
				map_start + sector_off, sectors_this);
			kfree_null(&buf);
			kfree_null(&info->l2_not_full);
			kfree_null(&info->l2_all_free);
			kfree_null(&info->l1_not_full);
			kfree_null(&info->l1_all_free);
			kfree_null(&info->l1_free_count);
			return false;
		}

		// Valid bits in this buffer (don’t run past end of volume)
		const uint64_t bits_in_buf = MIN(bytes_this * 8ULL, total_bits - bit_cursor);

		// Process 64-bit words of bits; invert so 1-bits mean "free".
		const uint64_t full_words = bits_in_buf >> 6;   // / 64
		const uint64_t tail_bits  = bits_in_buf & 63ULL;

		const uint64_t *wptr = (const uint64_t*)buf;

		uint64_t pos = bit_cursor; // global bit index for start of current word

		for (uint64_t wi = 0; wi < full_words; ++wi, ++wptr, pos += 64ULL) {
			uint64_t word = ~(*wptr); // free=1, used=0

			if (word == 0ULL) {
				continue; // all used, nothing to accumulate
			}

			// Distribute this word’s free bits across L1 groups it may span
			uint64_t remaining = 64ULL;
			while (word && remaining) {
				const uint64_t g = pos / G;
				if (g >= info->l1_groups) {
					// Safety; shouldn’t happen because bits_in_buf clamps to total_bits
					break;
				}
				const uint64_t group_end = (g + 1ULL) * G;
				const uint64_t in_group  = group_end - pos;          // bits to group boundary
				const uint64_t take      = MIN(remaining, in_group); // <=64

				// Mask off only the 'take' LSBs of 'word'
				const uint64_t mask = (take == 64ULL) ? ~0ULL : ((1ULL << take) - 1ULL);
				const uint64_t seg  = word & mask;

				if (seg) {
					const int add = __builtin_popcountll(seg);
					uint16_t *fc = &info->l1_free_count[g];
					const uint64_t group_start = g * G;
					const uint64_t group_size  = (group_start + G <= info->total_sectors)
								     ? G
								     : (info->total_sectors - group_start);
					if (*fc < group_size) {
						uint64_t room = group_size - *fc;
						*fc += (uint16_t)MIN((uint64_t)add, room);
					}
				}

				// Advance within word and possibly into next group
				word     >>= take;
				pos      += take;
				remaining -= take;
			}
		}

		if (tail_bits) {
			uint64_t tail = ((const uint64_t*)buf)[full_words];
			uint64_t word = ~tail;

			// Keep only 'tail_bits'
			const uint64_t mask = (1ULL << tail_bits) - 1ULL;
			word &= mask;

			uint64_t remaining = tail_bits;
			while (word && remaining) {
				const uint64_t g = pos / G;
				if (g >= info->l1_groups) break;

				const uint64_t group_end = (g + 1ULL) * G;
				const uint64_t in_group  = group_end - pos;
				const uint64_t take      = MIN(remaining, in_group);

				const uint64_t seg_mask = (take == 64ULL) ? ~0ULL : ((1ULL << take) - 1ULL);
				const uint64_t seg      = word & seg_mask;

				if (seg) {
					const int add = __builtin_popcountll(seg);
					uint16_t *fc = &info->l1_free_count[g];
					const uint64_t group_start = g * G;
					const uint64_t group_size  = (group_start + G <= info->total_sectors)
								     ? G
								     : (info->total_sectors - group_start);
					if (*fc < group_size) {
						uint64_t room = group_size - *fc;
						*fc += (uint16_t)MIN((uint64_t)add, room);
					}
				}

				word     >>= take;
				pos      += take;
				remaining -= take;
			}
		}

		bit_cursor += bits_in_buf;
		sector_off += sectors_this;
	}

	kfree_null(&buf);

	// Derive L1 bitsets from counters
	for (uint64_t g = 0; g < info->l1_groups; ++g) {
		const uint16_t fc = info->l1_free_count[g];
		bitset_set(info->l1_not_full, g, (fc > 0));

		const uint64_t group_start = g * G;
		const uint64_t group_size  = (group_start + G <= info->total_sectors)
					     ? G
					     : (info->total_sectors - group_start);
		bitset_set(info->l1_all_free, g, (fc == (uint16_t)group_size));
	}

	// Build L2 by folding over S children
	for (uint64_t sg = 0; sg < info->l2_groups; ++sg) {
		const uint64_t g0 = sg * S;
		const uint64_t g1 = MIN(g0 + S, info->l1_groups);

		bool any_free = false;
		bool all_full_groups_free = true;

		for (uint64_t g = g0; g < g1; ++g) {
			any_free |= bitset_get(info->l1_not_full, g);
			if (!bitset_get(info->l1_all_free, g)) {
				all_full_groups_free = false;
			}
		}

		bitset_set(info->l2_not_full, sg, any_free);
		bitset_set(info->l2_all_free, sg, all_full_groups_free);
	}

	dprintf("RFS: Built L1/L2: total_sectors=%lu, l1_groups=%lu, l2_groups=%lu\n",
		info->total_sectors, info->l1_groups, info->l2_groups);
	return true;
}


bool rfs_format(rfs_t* info) {
	if (!info) {
		return false;
	}

	uint64_t size_sectors = info->length / RFS_SECTOR_SIZE;

	rfs_description_block_padded_t description_block = { 0 };
	description_block.desc.identifier = RFS_ID;
	description_block.desc.sequence = 1;
	description_block.desc.creation_time = time(NULL);
	description_block.desc.free_space_map_checksum = 0;

	// Free space map: 1 bit per sector
	uint64_t map_bits   = size_sectors;
	uint64_t map_bytes  = (map_bits + 7) / 8;
	uint64_t map_sectors = (map_bytes + RFS_SECTOR_SIZE - 1) / RFS_SECTOR_SIZE;

	description_block.desc.free_space_map_length = map_sectors;
	description_block.desc.free_space_map_start  = size_sectors - map_sectors;
	description_block.desc.root_directory = 1;

	// Write description block (LBA 0)
	if (!rfs_write_device(info, 0, sizeof(description_block), &description_block.raw)) {
		dprintf("Failed to write RFS description block\n");
		return false;
	}

	// Write root directory entry (LBA 1)
	rfs_directory_entry_t dirent = { 0 };
	dirent.start.flags = RFS_FLAG_DIR_START;
	dirent.start.continuation = 0;
	dirent.start.parent = 0;
	dirent.start.sectors = 64;   // root dir cluster size
	if (!rfs_write_device(info, 1, sizeof(dirent), &dirent)) {
		dprintf("Failed to write RFS root directory entry\n");
		return false;
	}

	// e.g. 128 sectors per write = 64 KiB
	const size_t chunk = 128;
	uint8_t zerobuf[RFS_SECTOR_SIZE * 128] = { 0 };
	for (uint64_t i = 0; i < map_sectors; i += chunk) {
		size_t this_write = (map_sectors - i > chunk) ? chunk : (map_sectors - i);
		if (!rfs_write_device(info,
				      description_block.desc.free_space_map_start + i,
				      this_write * RFS_SECTOR_SIZE,
				      zerobuf)) {
			dprintf("Failed to clear free space map sector %lu\n", i);
			return false;
		}
	}


	// Save description block in memory
	info->desc = kmalloc(sizeof(rfs_description_block_padded_t));
	memcpy(info->desc, &description_block, sizeof(rfs_description_block_padded_t));

	// Mark first 65 sectors as used (desc block + root directory cluster)
	rfs_reserve_in_fs_map(info, 0, 65);

	return true;
}


bool read_rfs_description_block(rfs_t* info)
{
	if (!info) {
		return 0;
	}
	rfs_description_block_padded_t* description_block = kmalloc(sizeof(rfs_description_block_padded_t));
	if (!description_block) {
		return 0;
	}
	if (!rfs_read_device(info, 0, RFS_SECTOR_SIZE, description_block)) {
		kprintf("RFS: Could not read RFS description block!\n");
		kfree_null(&description_block);
		return false;
	}
	if (description_block->desc.identifier != RFS_ID) {
		dprintf("Identifier %lx is not %lx (\"RetroFS1\")\n", description_block->desc.identifier, RFS_ID);
		kfree_null(&description_block);
		return false;
	}
	info->desc = &description_block->desc;

	return true;
}

rfs_t* rfs_mount_volume(const char* device_name)
{
	if (!device_name) {
		return NULL;
	}
	char found_guid[64];
	rfs_t* info = kmalloc(sizeof(rfs_t));
	if (!info) {
		return NULL;
	}
	info->dev = find_storage_device(device_name);
	if (!info->dev || info->dev->block_size != RFS_SECTOR_SIZE) {
		kprintf("%s: Device not suitable for RFS\n", info->dev ? info->dev->name : "<unknown>");
		kfree_null(&info);
		return 0;
	}

	uint64_t start = 0, length = 0;

	bool success = false;
	uint8_t partitionid = 0;
	if (!find_partition_of_type(device_name, 0xFF, found_guid, RFS_GPT_GUID, &partitionid, &start, &length)) {
		/* No partition found, attempt to mount the volume as whole disk */
		info->start = 0;
		info->length = info->dev->size * info->dev->block_size;
		success = read_rfs_description_block(info);
		if (success) {
			kprintf("Found RFS volume, device %s\n", device_name);
		}
	} else {
		info->start = start;
		info->length = length * info->dev->block_size;
		/* 0xFF means we got a GPT partition not MBR partition; only GPT partitions are supported for RFS. */
		if (partitionid == 0xFF) {
			success = read_rfs_description_block(info);
			if (success) {
				kprintf("Found RFS GPT partition, device %s\n", device_name);
			}
		}
	}
	if (!success) {
		kfree_null(&info);
		return NULL;
	}
	if (!rfs_build_level_caches(info)) {
		dprintf("Failed to build RFS L1/L2 caches\n");
		kfree_null(&info);
		return NULL;
	}
	return info;
}


int rfs_attach(const char* device_name, const char* path)
{
	rfs_t* rfs = rfs_mount_volume(device_name);
	if (rfs) {
		return attach_filesystem(path, rfs_fs, rfs);
	}
	return 0;
}

void* rfs_get_directory(void* t) {
	return NULL;
}

bool rfs_unlink_file(void* dir, const char* name) {
	return false;
}

bool rfs_read_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer) {
	return false;
}

uint64_t rfs_create_file(void* dir, const char* name, size_t size) {
	return 0;
}

uint64_t rfs_create_directory(void* dir, const char* name) {
	return 0;
}

bool rfs_unlink_dir(void* dir, const char* name) {
	return false;
}

bool rfs_write_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer) {
	return false;
}

bool rfs_truncate_file(void* f, size_t length) {
	return false;
}

void init_rfs() {
	rfs_fs = kmalloc(sizeof(filesystem_t));
	if (!rfs_fs) {
		return;
	}
	strlcpy(rfs_fs->name, "rfs", 31);
	rfs_fs->mount = rfs_attach;
	rfs_fs->getdir = rfs_get_directory;
	rfs_fs->readfile = rfs_read_file;
	rfs_fs->createfile = rfs_create_file;
	rfs_fs->truncatefile = rfs_truncate_file;
	rfs_fs->createdir = rfs_create_directory;
	rfs_fs->writefile = rfs_write_file;
	rfs_fs->rm = rfs_unlink_file;
	rfs_fs->rmdir = rfs_unlink_dir;
	register_filesystem(rfs_fs);
}