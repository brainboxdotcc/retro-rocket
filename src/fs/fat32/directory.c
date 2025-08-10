#include "kernel.h"

/**
 * @brief Free a fs_directory_entry_t* list returned by parse_fat32_directory()
 *
 * @param list
 */
void free_fat32_directory(fs_directory_entry_t* list)
{
	while (list) {
		kfree_null(&list->filename);
		kfree_null(&list->alt_filename);
		fs_directory_entry_t* next = list->next;
		kfree_null(&list);
		list = next;
	}
}

fs_directory_entry_t* parse_fat32_directory(fs_tree_t* tree, fat32_t* info, uint32_t cluster)
{
	unsigned char* buffer = kmalloc(info->clustersize);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	fs_directory_entry_t* list = NULL;
	lfn_t lfns[256] = { 0 };
	int16_t highest_lfn_order = -1;

	dprintf("Get fat32 dir from cluster %u\n", cluster);

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			kfree_null(&buffer);
			dprintf("Couldn't read cluster %u\n", cluster);
			return NULL;
		}

		dprintf("Cluster:\n");
		dump_hex(buffer, info->clustersize);

		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; /* max of info->clustersize / 32 file entries per cluster */

		while (entries++ < info->clustersize / 32) {
			/* End of directory */
			if (entry->name[0] == 0x00) {
				dprintf("End of dir, null name\n");
				break;
			}

			/* Skip deleted entries (0xE5) */
			if ((unsigned char)entry->name[0] == 0xE5) {
				dprintf("Skip deleted item\n");
				goto next_entry;
			}

			/* Skip dot and dotdot */
			if (entry->name[0] == '.' && (entry->name[1] == ' ' || entry->name[1] == '.')) {
				/* Any pending LFN sequence is invalidated */
				highest_lfn_order = -1;
				memset(&lfns, 0, sizeof(lfns));
				goto next_entry;
			}

			if ((entry->attr & ATTR_VOLUME_ID) && info->volume_name == NULL) {
				/* Volume label entry (don’t require ATTR_ARCHIVE) */
				char name[13];
				char dotless[13];
				parse_short_name(entry, name, dotless);
				info->volume_name = strdup(dotless);
				highest_lfn_order = -1;
				memset(&lfns, 0, sizeof(lfns));
				dprintf("volume id\n");
			} else if ((entry->attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
				/* LFN entry: stash by sequence index (mask off 0x40) */
				lfn_t* lfn = (lfn_t*)entry;
				uint8_t idx = (uint8_t)(lfn->order & 0x1F);
				if (idx > 0 && idx < 256) {
					memcpy(&lfns[idx], lfn, sizeof(lfn_t));
					if ((int16_t)idx > highest_lfn_order) {
						highest_lfn_order = (int16_t)idx;
					}
				}
				dprintf("long name\n");
			} else {
				/* SFN (regular file/dir) — build entry, optionally with LFN */
				char sfn[13];
				char sfn_dotless[13];
				dprintf("short name\n");
				parse_short_name(entry, sfn, sfn_dotless);

				fs_directory_entry_t* file = kmalloc(sizeof(fs_directory_entry_t));
				if (!file) {
					fs_set_error(FS_ERR_OUT_OF_MEMORY);
					kfree_null(&buffer);
					return NULL;
				}

				if (highest_lfn_order > -1) {
					/* Assemble long name from collected LFN parts */
					char longname[14 * (highest_lfn_order + 1)];
					char* nameptr = longname;

					for (int i = 1; i <= highest_lfn_order; ++i) {
						if (lfns[i].first[0] == 0 || lfns[i].first[0] == 0xffff) {
							continue;
						}
						nameptr = parse_shitty_lfn_entry(nameptr, lfns[i].first, 5);
						nameptr = parse_shitty_lfn_entry(nameptr, lfns[i].second, 6);
						nameptr = parse_shitty_lfn_entry(nameptr, lfns[i].third, 2);
					}
					*nameptr++ = 0;

					file->filename = strdup(longname);
					highest_lfn_order = -1;
					memset(&lfns, 0, sizeof(lfns));
				} else {
					file->filename = strdup(sfn);
				}

				file->alt_filename = strdup(sfn);
				file->lbapos = (uint64_t)(((uint64_t)entry->first_cluster_hi << 16) |
							  (uint64_t)entry->first_cluster_lo);
				file->directory = tree;
				file->flags = 0;
				file->size = entry->size;

				if (entry->attr & ATTR_DIRECTORY) {
					file->flags |= FS_DIRECTORY;
				}

				file->next = list;
				list = file;

				dprintf("Append name\n");
			}

			next_entry:
			bufferoffset += 32;
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		if (highest_lfn_order > -1) {
			/* Trailing or broken LFN without a matching SFN */
			fs_set_error(FS_ERR_BROKEN_DIRECTORY);
			highest_lfn_order = -1;
			memset(&lfns, 0, sizeof(lfns));
		}

		/* advance to next cluster in chain until EOF */
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			dprintf("next eoc\n");
			break;
		} else {
			dprintf("next %u\n", nextcluster);
			cluster = nextcluster;
		}
	}
	dprintf("End list\n");

	kfree_null(&buffer);
	return list;
}


void insert_entries_at(bool grow, fat32_t* info, uint32_t entries, uint32_t cluster, uint8_t* buffer, int bufferoffset, directory_entry_t* short_entry, directory_entry_t* new_entries, int entry_count)
{
	if (grow) {
		/* Insert new cluster into directory and point at it */
		uint32_t next_free = find_next_free_fat_entry(info);
		set_fat_entry(info, cluster, next_free);
		set_fat_entry(info, next_free, CLUSTER_END);
		amend_free_count(info, -1);
		cluster = next_free;
		/* Zero the memory copy of the cluster */
		memset(buffer, 0, info->clustersize);
		bufferoffset = 0;
		entries = 0;
	}


	if (!read_cluster(info, cluster, buffer)) {
		return;
	}

	directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);

	/* Write LFN entries */
	do {
		/**
		 * XXX: It is important to write the list of LFN entries BACKWARDS,
		 * so that the final entry is first with ATTR_LFN_LAST_ENTRY bit set
		 * in its flags. The spec doesn't say this is important, and in theory
		 * we should be able to reassemble the long name with its parts in any
		 * order but linux at least is strict in this expectation and will only
		 * display part of the name or none, if the order is not as output
		 * traditionally by windows.
		 */
		for (; entries < info->clustersize / 32 && entry_count > -1; ++entries) {
			memcpy(entry, &new_entries[entry_count--], sizeof(directory_entry_t));
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		if (entries == info->clustersize / 32) {
			fs_set_error(FS_ERR_LFN_TOO_LONG);
			dprintf("fat32: insert_entries_at() overlaps cluster edge, should not happen - LFN too long?\n");
			return;
		}

	} while (entry_count > -1 && entries != info->clustersize / 32);

	if (entries < info->clustersize / 32) {
		memcpy(entry, short_entry, sizeof(directory_entry_t));
	}

	if (!write_cluster(info, cluster, buffer)) {
		dprintf("Write storage failed when extending directory\n");
	}
}

uint64_t fat32_create_directory(void* dir, const char* name)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t parent_dir_cluster = treeitem->lbapos ? treeitem->lbapos : info->rootdircluster;
	uint8_t* buffer = kmalloc(info->clustersize);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}
	int bufferoffset = 0;
	directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);

	uint64_t cluster = fat32_internal_create_file(dir, name, 0, ATTR_DIRECTORY);
	if (cluster == 0) {
		kfree_null(&buffer);
		return 0;
	}

	if (!read_cluster(info, cluster, buffer)) {
		kfree_null(&buffer);
		return 0;
	}

	/* Create special '.' and '..' entries in the directory, required for directory to be considered valid.
	 * '.' entry points to own cluster, and '..' entry points to cluster of parent.
	 */
	strlcpy(entry->name, ".          ", 11);
	entry->attr = ATTR_DIRECTORY;
	entry->size = 0;
	entry->first_cluster_hi = (cluster >> 16) & 0xffff;
	entry->first_cluster_lo = cluster & 0xffff;
	bufferoffset += 32;
	entry = (directory_entry_t*)(buffer + bufferoffset);
	strlcpy(entry->name, "..         ", 11);
	entry->attr = ATTR_DIRECTORY;
	entry->size = 0;
	entry->first_cluster_hi = (parent_dir_cluster >> 16) & 0xffff;
	entry->first_cluster_lo = parent_dir_cluster & 0xffff;
	if (!write_cluster(info, cluster, buffer)) {
		kfree_null(&buffer);
		return 0;
	}

	kfree_null(&buffer);
	return cluster;
}

void* fat32_get_directory(void* t)
{
	if (!t) {
		dprintf("*** BUG *** fat32_get_directory: null fs_tree_t*!\n");
		return NULL;
	}
	fs_tree_t* treeitem = (fs_tree_t*)t;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	dprintf("treeitem->lbapos = %lu info->rootdircluster = %u\n", treeitem->lbapos, info->rootdircluster);
	return parse_fat32_directory(treeitem, info, treeitem->lbapos ? treeitem->lbapos : info->rootdircluster);
}

bool fat32_unlink_dir(void* dir, const char* name)
{
	if (!dir || !name) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fs_directory_entry_t* iter = NULL;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t dir_cluster = treeitem->lbapos;

	iter = parse_fat32_directory(treeitem, info, dir_cluster);
	if (iter) {
		/* Directory not empty */
		fs_set_error(FS_ERR_DIRECTORY_NOT_EMPTY);
		free_fat32_directory(iter);
		return false;
	}
	free_fat32_directory(iter);
	return fat32_unlink_file(dir, name);
}

