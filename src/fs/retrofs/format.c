#include <kernel.h>
#include <retrofs.h>

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
	dirent.start.sectors = RFS_DEFAULT_DIR_SIZE;   // root dir cluster size
	if (!rfs_write_device(info, 1, sizeof(dirent), &dirent)) {
		dprintf("Failed to write RFS root directory entry\n");
		return false;
	}

	// RFS_MAP_READ_CHUNK_SECTORS per write = 64K @ 128 sectors
	const size_t chunk = RFS_MAP_READ_CHUNK_SECTORS;
	uint8_t zerobuf[RFS_SECTOR_SIZE * RFS_MAP_READ_CHUNK_SECTORS] = { 0 };
	for (uint64_t i = 0; i < map_sectors; i += chunk) {
		size_t this_write = (map_sectors - i > chunk) ? chunk : (map_sectors - i);
		if (!rfs_write_device(info, description_block.desc.free_space_map_start + i, this_write * RFS_SECTOR_SIZE, zerobuf)) {
			dprintf("Failed to clear free space map sector %lu\n", i);
			return false;
		}
	}


	// Save description block in memory
	info->desc = kmalloc(sizeof(rfs_description_block_padded_t));
	memcpy(info->desc, &description_block, sizeof(rfs_description_block_padded_t));

	// Mark first 65 sectors as used (desc block + root directory)
	rfs_mark_extent(info, 0, RFS_DEFAULT_DIR_SIZE + 1, true);

	// Mark the free-space map *itself* as used
	rfs_mark_extent(info, description_block.desc.free_space_map_start, description_block.desc.free_space_map_length, true);

	return true;
}


