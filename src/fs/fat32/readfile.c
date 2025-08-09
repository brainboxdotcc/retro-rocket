#include "kernel.h"

bool fat32_read_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer)
{
	if (!f || !buffer) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint32_t clustercount = 0;
	uint32_t first = 1;
	uint32_t amount_read = 0;
	unsigned char* clbuf = kmalloc(info->clustersize);
	if (!clbuf) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}

	dprintf("fat32_read_file: lbapos=%08x\n", cluster);

	// Advance to start when start != 0, for fseek()
	while ((clustercount++ < start / info->clustersize) && cluster < CLUSTER_BAD) {
		cluster = get_fat_entry(info, cluster);
	}

	if (cluster >= CLUSTER_BAD) {
		fs_set_error(FS_ERR_BAD_CLUSTER);
		return false;
	}

	while (true) {
		if (!read_cluster(info, cluster, clbuf)) {
			kfree_null(&clbuf);
			return false;
		}

		int to_read = length;
		if (length > info->clustersize) {
			to_read = info->clustersize;
		}
		if (first == 1) {
			/* Special case where start != 0 */
			memcpy(buffer, clbuf + (start % info->clustersize), to_read);
		} else {
			memcpy(buffer, clbuf, to_read);
		}

		buffer += to_read;
		amount_read += to_read;
		first = 0;

		cluster = get_fat_entry(info, cluster);

		if (cluster >= CLUSTER_BAD || amount_read >= length) {
			break;
		}
	}


	kfree_null(&clbuf);
	return true;
}

