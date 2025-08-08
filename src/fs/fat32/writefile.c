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
		return false;
	}

	if (start + length > file->size) {
		// File size must be extended to meet this requirement
		if (!fat32_extend_file(f, start + length - file->size)) {
			dprintf("Couldn't extend file\n");
			kfree_null(&clbuf);
			return false;
		}
	}

	uint64_t start_offset = start % info->clustersize;

	dprintf("Start: %ld length %d cluster %08x\n", start, length, cluster);

	while (cluster < CLUSTER_BAD) {
		dprintf("Current pos: %ld cluster %08x\n", current_pos, cluster);
		uint64_t start_of_block = current_pos;
		uint64_t end_of_block = current_pos + info->clustersize;
		uint64_t start_of_write = start;
		uint64_t end_of_write = start + length;
		if (start_of_block <= end_of_write || end_of_block >= start_of_write) {
			dprintf("In range, writing\n");
			if (!read_cluster(info, cluster, clbuf)) {
				kfree_null(&clbuf);
				dprintf("Didnt read cluster %d\n", cluster);
				return false;
			}
			int64_t to_write = length;
			if (length > info->clustersize) {
				to_write = info->clustersize;
			}
			memcpy(clbuf + start_offset, buffer, to_write);
			start_offset = 0;
			dprintf("Amended cluster:\n");
			if (!write_cluster(info, cluster, clbuf)) {
				dprintf("Write failure in fat32_write_file cluster=%08x\n", cluster);
				kfree_null(&clbuf);
				return false;
			}
			buffer += to_write;
		}
		cluster = get_fat_entry(info, cluster);
		current_pos += info->clustersize;
		dprintf("New current pos: %ld new cluster %08x\n", current_pos, cluster);
	}
	kfree_null(&clbuf);
	return true;
}
