#include "kernel.h"

bool fat32_write_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint64_t current_pos = 0;
	unsigned char* clbuf = kmalloc(info->clustersize);
	if (!clbuf) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}

	if (start + length > file->size) {
		// File size must be extended to meet this requirement
		if (!fat32_extend_file(f, start + length - file->size)) {
			fs_set_error(FS_ERR_NO_SPACE);
			kfree_null(&clbuf);
			return false;
		}
	}

	uint64_t start_offset = start % info->clustersize;

	while (cluster < CLUSTER_BAD) {
		uint64_t start_of_block = current_pos;
		uint64_t end_of_block = current_pos + info->clustersize;
		uint64_t start_of_write = start;
		uint64_t end_of_write = start + length;
		if (start_of_block <= end_of_write || end_of_block >= start_of_write) {
			if (!read_cluster(info, cluster, clbuf)) {
				kfree_null(&clbuf);
				return false;
			}
			int64_t to_write = length;
			if (length > info->clustersize) {
				to_write = info->clustersize;
			}
			memcpy(clbuf + start_offset, buffer, to_write);
			start_offset = 0;
			if (!write_cluster(info, cluster, clbuf)) {
				kfree_null(&clbuf);
				return false;
			}
			buffer += to_write;
		}
		cluster = get_fat_entry(info, cluster);
		current_pos += info->clustersize;
	}
	kfree_null(&clbuf);
	return true;
}
