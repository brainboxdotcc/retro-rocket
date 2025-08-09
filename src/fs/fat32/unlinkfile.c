#include "kernel.h"

bool fat32_unlink_file(void* dir, const char* name)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fs_directory_entry_t* iter = NULL, *parsed_dir = NULL;
	uint32_t cluster = CLUSTER_END;
	uint32_t file_start = CLUSTER_END;
	int32_t freed = 0;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t dir_cluster = treeitem->lbapos ? treeitem->lbapos : info->rootdircluster;

	parsed_dir = iter = parse_fat32_directory(treeitem, info, dir_cluster);
	for (; iter; iter = iter->next) {
		if (!strcmp(iter->filename, name)) {
			file_start = cluster = iter->lbapos;
			break;
		}
	}

	if (file_start == CLUSTER_END) {
		fs_set_error(FS_ERR_NO_SUCH_FILE);
		free_fat32_directory(parsed_dir);
		return false;
	}

	/* Clear out the clusters to CLUSTER_FREE */
	do {
		uint32_t cur = cluster;
		cluster = get_fat_entry(info, cluster);
		freed++;
		if (cur < CLUSTER_BAD) {
			if (!set_fat_entry(info, cur, CLUSTER_FREE)) {
				free_fat32_directory(parsed_dir);
				fs_set_error(FS_ERR_BAD_CLUSTER);
				return false;
			}
		}
	} while (cluster < CLUSTER_BAD);

	amend_free_count(info, freed);

	/* Remove related entries from directory */
	uint8_t* buffer = kmalloc(info->clustersize);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}
	directory_entry_t* entry_lfn_start = NULL;
	cluster = dir_cluster;
	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			kfree_null(&buffer);
			free_fat32_directory(parsed_dir);
			return false;
		}

		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster
		while (entries++ < info->clustersize / 32) {
			if (!(entry->name[0] & 0x80) && entry->name[0] != 0) {
				char name[13];
				char dotless[13];
				parse_short_name(entry, name, dotless);

				if (name[0] == 0)
					break;
				if (name[0] != '.') {
					if (entry->attr == ATTR_LONG_NAME) {
						if (entry_lfn_start == NULL) {
							entry_lfn_start = entry;
						}
					} else {
						uint32_t this_start = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
						if (file_start == this_start) {
							/* Found the directory entry we are removing, mark its entry as deleted */
							*(entry->name) = DELETED_ENTRY;
							/* Mark related lfn entries as deleted */
							while (entry_lfn_start && entry_lfn_start < entry) {
								*(entry_lfn_start->name) = DELETED_ENTRY;
								entry_lfn_start += sizeof(directory_entry_t);
							}
							write_cluster(info, cluster, buffer);
							kfree_null(&buffer);
							free_fat32_directory(parsed_dir);
							return true;
						}
						entry_lfn_start = NULL;
					}
				}
			}
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			break;
		} else {
			cluster = nextcluster;
		}
	}
	kfree_null(&buffer);
	free_fat32_directory(parsed_dir);
	return true;
}
