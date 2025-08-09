#include "kernel.h"

bool read_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	return read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer);
}

bool write_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	return write_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer);
}

uint64_t cluster_to_lba(fat32_t* info, uint32_t cluster)
{
	if (!info) {
		fs_set_error(FS_ERR_VFS_DATA);
		return 0;
	}
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}
	uint64_t FirstDataSector = info->reservedsectors + (info->numberoffats * info->fatsize);
	uint64_t FirstSectorofCluster = ((cluster - 2) * (info->clustersize / sd->block_size) ) + FirstDataSector;
	return info->start + FirstSectorofCluster;
}

