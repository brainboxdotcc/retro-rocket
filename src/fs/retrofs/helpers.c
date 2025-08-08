#include <kernel.h>
#include <retrofs.h>

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

int rfs_write_device(rfs_t* rfs, uint64_t start_sectors, uint64_t size_bytes, const void* buffer)
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