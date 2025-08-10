#include "kernel.h"

bool read_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	if (!info || !buffer) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}
	uint64_t lba = cluster_to_lba(info, cluster);
	if (lba == 0) {
		/* cluster_to_lba already set fs_error */
		return false;
	}
	return read_storage_device(info->device_name, lba, info->clustersize, buffer);
}

bool write_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	if (!info || !buffer) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}
	uint64_t lba = cluster_to_lba(info, cluster);
	if (lba == 0) {
		/* cluster_to_lba already set fs_error */
		return false;
	}
	return write_storage_device(info->device_name, lba, info->clustersize, buffer);
}

uint64_t cluster_to_lba(fat32_t* info, uint32_t cluster)
{
	if (!info) {
		fs_set_error(FS_ERR_VFS_DATA);
		return 0;
	}

	/* FAT32 data clusters start at 2; 0/1 are reserved */
	if (cluster < 2) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}

	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd || sd->block_size == 0) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}

	/* clustersize is in BYTES; convert to sectors-per-cluster */
	if ((info->clustersize % sd->block_size) != 0) {
		/* Geometry mismatch: cluster size not an integer multiple of device block */
		fs_set_error(FS_ERR_INVALID_GEOMETRY);
		return 0;
	}

	const uint64_t spc = (uint64_t)info->clustersize / (uint64_t)sd->block_size;

	/* First data sector = reserved + (num_fats * fat_size) — all in SECTORS */
	const uint64_t first_data_sector =
		(uint64_t)info->reservedsectors + ((uint64_t)info->numberoffats * (uint64_t)info->fatsize);

	/* Sector index of this cluster within the partition (still in SECTORS) */
	const uint64_t first_sector_of_cluster =
		((uint64_t)(cluster - 2U) * spc) + first_data_sector;

	/* info->start is the partition LBA base in device sectors */
	return (uint64_t)info->start + first_sector_of_cluster;
}


