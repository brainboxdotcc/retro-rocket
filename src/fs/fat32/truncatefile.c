#include "kernel.h"

bool fat32_truncate_file(void* f, size_t length)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint64_t current_pos = 0;
	bool first = true;
	unsigned char* clbuf = kmalloc(info->clustersize);
	if (!clbuf) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}

	/* Walk cluster chain until we find the last one we want to keep */
	while (cluster < CLUSTER_BAD && current_pos <= length) {
		cluster = get_fat_entry(info, cluster);
		current_pos += info->clustersize;
	}
	while (cluster < CLUSTER_BAD) {
		set_fat_entry(info, cluster, first ? CLUSTER_END : CLUSTER_FREE);
		first = false;
		cluster = get_fat_entry(info, cluster);
	}
	kfree_null(&clbuf);

	/* Amend size in directory */
	uint8_t* buffer = kmalloc(info->clustersize);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}
	cluster = file->directory->lbapos ? file->directory->lbapos : info->rootdircluster;

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			kfree_null(&buffer);
			return false;
		}
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster

		while (entries++ < info->clustersize / 32) {
			uint32_t this_file_cluster = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
			if (this_file_cluster == file->lbapos && entry->name[0] != 0 && !(entry->name[0] & 0x80) && entry->attr != ATTR_LONG_NAME) {
				entry->size = length;
				file->size = length;
				write_cluster(info, cluster, buffer);
				kfree_null(&buffer);
				return true;
			}
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			fs_set_error(FS_ERR_NO_SUCH_FILE);
			kfree_null(&buffer);
			return false;
		} else {
			cluster = nextcluster;
		}
	}
	kfree_null(&buffer);
	return false;
}
