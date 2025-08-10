#include "kernel.h"

bool fat32_extend_file(void* f, uint32_t size)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;
	uint32_t last_cluster = CLUSTER_END;
	bool alloc_more = true;

	uint32_t cluster = file->lbapos;

	/* Work out if we actually need to grow or if there is space in the slack */
	uint32_t size_cl = 0;
	while (cluster < CLUSTER_BAD) {
		size_cl += info->clustersize;
		cluster = get_fat_entry(info, cluster);
	}
	if (size_cl >= file->size + size) {
		alloc_more = false;
	}
	cluster = file->lbapos;

	/* Allocate cluster chain to cover file length */
	uint8_t* blank_cluster = kmalloc(info->clustersize);
	if (!blank_cluster) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}

	/* Calculate how many clusters are needed for this size of file */
	uint32_t size_in_clusters = size / info->clustersize;
	while (size_in_clusters * info->clustersize < size) {
		size_in_clusters++;
	}
	if (size_in_clusters < 1) {
		size_in_clusters++;
	}


	if (alloc_more) {
		while (cluster < CLUSTER_BAD) {
			last_cluster = cluster;
			cluster = get_fat_entry(info, cluster);
		}
		/* last_cluster now points at last cluster of file */

		/* Newly allocated clusters will be filled with nulls */
		memset(blank_cluster, 0, info->clustersize);
		amend_free_count(info, -size_in_clusters);
		while (size_in_clusters > 0) {
			uint32_t cluster = find_next_free_fat_entry(info);
			if (cluster < CLUSTER_BAD) {
				set_fat_entry(info, cluster, CLUSTER_END);
				/* Zero the clusters as we allocate them to the file */
				if (!write_cluster(info, cluster, blank_cluster)) {
					kfree_null(&blank_cluster);
					return false;
				}
			}
			if (last_cluster < CLUSTER_BAD) {
				set_fat_entry(info, last_cluster, cluster);
			}
			last_cluster = cluster;
			size_in_clusters--;
		}
	}
	kfree_null(&blank_cluster);

	/* Amend the file's size in its directory entry */
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
				entry->size += size;
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
}

uint64_t fat32_internal_create_file(void* dir, const char* name, size_t size, uint8_t attributes)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fs_directory_entry_t* iter = NULL, *parsed_dir = NULL;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t dir_cluster = treeitem->lbapos ? treeitem->lbapos : info->rootdircluster;

	/* check that a file doesn't already exist with this name */
	parsed_dir = iter = parse_fat32_directory(treeitem, info, dir_cluster);
	for (; iter; iter = iter->next) {
		if (!strcmp(iter->filename, name)) {
			fs_set_error(FS_ERR_FILE_EXISTS);
			free_fat32_directory(parsed_dir);
			return 0;
		}
	}

	/* Calculate how many clusters are needed for this size of file */
	uint32_t size_in_clusters = size / info->clustersize;
	while (size_in_clusters * info->clustersize < size) {
		size_in_clusters++;
	}
	if (size_in_clusters < 1) {
		size_in_clusters++;
	}

	/* Allocate cluster chain to cover file length */
	uint32_t last_cluster = CLUSTER_END;
	uint32_t first_allocated_cluster = CLUSTER_END;
	uint8_t* blank_cluster = kmalloc(info->clustersize);
	if (!blank_cluster) {
		return 0;
	}
	/* Initialise the clusters to full of nulls */
	memset(blank_cluster, 0, info->clustersize);
	amend_free_count(info, (int)-size_in_clusters);
	while (size_in_clusters > 0) {
		uint32_t cluster = find_next_free_fat_entry(info);
		if (cluster != CLUSTER_END) {
			set_fat_entry(info, cluster, CLUSTER_END);
			if (first_allocated_cluster == CLUSTER_END) {
				first_allocated_cluster = cluster;
			}
			/* Zero the clusters as we allocate them to the new file */
			if (!write_cluster(info, cluster, blank_cluster)) {
				kfree_null(&blank_cluster);
				free_fat32_directory(parsed_dir);
				return 0;
			}
		} else {
			kfree_null(&blank_cluster);
			free_fat32_directory(parsed_dir);
			fs_set_error(FS_ERR_NO_SPACE);
			return 0;
		}
		if (last_cluster != CLUSTER_END) {
			set_fat_entry(info, last_cluster, cluster);
		}
		last_cluster = cluster;
		size_in_clusters--;
	}
	kfree_null(&blank_cluster);

	/* Add directory entry (including lfn) */
	uint8_t* buffer = kmalloc(info->clustersize);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	uint32_t cluster = dir_cluster;
	size_t entry_count = 0;
	directory_entry_t* new_entries = NULL;
	directory_entry_t short_entry = { 0 };

	build_lfn_chain(name, parsed_dir, &short_entry, &new_entries, &entry_count);
	if (!new_entries) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	short_entry.size = size;
	short_entry.nt = 0;
	short_entry.attr = attributes;
	short_entry.first_cluster_hi = (first_allocated_cluster >> 16) & 0xffff;
	short_entry.first_cluster_lo = first_allocated_cluster & 0xffff;
	/* TODO: Set dates here */

	int32_t start_of_space_bufferoffset = -1;
	uint32_t start_of_space_cluster = 0;
	uint32_t space_size = -1;

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			kfree_null(&buffer);
			free_fat32_directory(parsed_dir);
			kfree_null(&new_entries);
			return 0;
		}
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster
		space_size = 0;
		start_of_space_bufferoffset = -1;

		while (entries++ < info->clustersize / 32) {
			/* Hunt for a space with entry_count+1 contiguous empty dir entries */
			if ((entry->name[0] & 0x80) || entry->name[0] == 0) {
				if (start_of_space_bufferoffset == -1) {
					start_of_space_bufferoffset = bufferoffset;
					start_of_space_cluster = cluster;
					space_size = 0;
				}
				space_size++;
				if (space_size > entry_count + 1 && entries + entry_count + 1 < info->clustersize / 32) {
					insert_entries_at(false, info, entries - 1, start_of_space_cluster, buffer, start_of_space_bufferoffset, &short_entry, new_entries, entry_count);
					free_fat32_directory(parsed_dir);
					kfree_null(&new_entries);
					kfree_null(&buffer);
					return first_allocated_cluster;
				}
			} else {
				start_of_space_bufferoffset = -1;
				start_of_space_cluster = 0;
				space_size = 0;
			}
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			insert_entries_at(true, info, entries - 1, cluster, buffer, 0, &short_entry, new_entries, entry_count);
			free_fat32_directory(parsed_dir);
			kfree_null(&new_entries);
			kfree_null(&buffer);
			return first_allocated_cluster;
		} else {
			cluster = nextcluster;
		}
	}
	fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
	kfree_null(&buffer);
	free_fat32_directory(parsed_dir);
	kfree_null(&new_entries);
	return 0;
}

uint64_t fat32_create_file(void* dir, const char* name, size_t size)
{
	return fat32_internal_create_file(dir, name, size, 0);
}
