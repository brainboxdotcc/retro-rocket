#include <kernel.h>

static filesystem_t* fat32_fs = NULL;

uint64_t cluster_to_lba(fat32_t* info, uint32_t cluster);
uint32_t get_fat_entry(fat32_t* info, uint32_t cluster);
bool set_fat_entry(fat32_t* info, uint32_t cluster, uint32_t value);
bool fat32_read_file(void* file, uint64_t start, uint32_t length, unsigned char* buffer);
int fat32_attach(const char* device_name, const char* path);
bool fat32_unlink_file(void* dir, const char* name);
uint32_t find_next_free_fat_entry(fat32_t* info);
void amend_free_count(fat32_t* info, int adjustment);
bool fat32_extend_file(void* f, uint32_t size);

bool read_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	return read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer);
}

bool write_cluster(fat32_t* info, uint32_t cluster, void* buffer)
{
	return write_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer);
}

/**
 * @brief Reads a chunk of a long filename from a directory entry.
 * 
 * WHY THE HELL DID MICROSOFT MAKE THIS IN SUCH A WEIRD WAY?!
 * Who in their right mind thought it sensible to split up a name
 * across multiple different tiny arrays, and why is it 16 bit unicode
 * instead of UTF-8 and WHY is 0xffff considered an invalid character?!
 * Does NOT compute!!!
 * 
 * @param nameptr pointer to name we are appending to
 * @param wide_chars wide characters to append
 * @param n_wide_chars number of wide characters to append
 * @return char* new name pointer
 */
char* parse_shitty_lfn_entry(char* nameptr, uint16_t* wide_chars, uint8_t n_wide_chars)
{
	for (int x = 0; x < n_wide_chars; ++x) {
		if (wide_chars[x] && wide_chars[x] != 0xffff) {
			if (wide_chars[x] < 0x80) {
				*nameptr++ = (char)wide_chars[x];
			} else {
				*nameptr++ = '?';
			}
		}
	}
	return nameptr;
}

uint8_t lfn_checksum(unsigned char* short_name)
{
	uint8_t sum = 0;
	for (uint16_t short_name_len = 11; short_name_len; --short_name_len) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *short_name++;
	}
	return sum;
}

void make_short(directory_entry_t* entry, const char* long_name, fs_directory_entry_t* current)
{
	memset(entry->name, ' ', 11);
	size_t end_number = 1;
	size_t x = 0;
	size_t n = 0;
	for (n = 0; n < 6; ++n) {
		while (!isalnum(long_name[x])) x++;
		if (long_name[x]) {
			entry->name[n] = toupper(long_name[x++]);
		}
	}
	entry->name[n++] = '~';
	/* Temporarily null terminate */
	entry->name[n + 2] = 0;
	bool found = false;
	do {
		found = false;
		entry->name[n] = (end_number / 10) + '0';
		entry->name[n + 1] = (end_number % 10) + '0';
		for (fs_directory_entry_t* e = current; e; e = e->next) {
			if (!memcmp(e->alt_filename, entry->name, 11)) {
				end_number++;
				found = true;
			}
		}
		if (end_number > 99) {
			return;
		}
	} while (found);
	entry->name[n + 2] = ' ';
}

#define FILL_UCS2(remain, number_chars, offset, member) \
	if (remain > offset) { \
		for (size_t n = 0; n < number_chars; ++n) { \
			lfn[*entry_count].member[n] = 0; \
			if (n + offset < remaining) { \
				lfn[*entry_count].member[n] = filename[n + offset]; \
			} \
		} \
	}

void build_lfn_chain(const char* filename, fs_directory_entry_t* current, directory_entry_t* short_entry, directory_entry_t** entries, size_t* entry_count)
{
	if (entry_count == NULL) {
		return;
	}
	*entry_count = 0;
	size_t increment = 1;
	make_short(short_entry, filename, current);
	*entries = kmalloc(sizeof(lfn_t) * 65);
	lfn_t* lfn = (lfn_t*)*entries;
	memset(lfn, 0, sizeof(lfn_t) * 65);
	uint8_t checksum = lfn_checksum((unsigned char*)short_entry->name);
	for (; *filename;) {
		lfn[*entry_count].attributes = ATTR_LONG_NAME;
		lfn[*entry_count].order = increment++;
		lfn[*entry_count].entry_type = 0;
		lfn[*entry_count].reserved = 0;
		lfn[*entry_count].checksum = checksum;
		size_t remaining = strlen(filename);
		FILL_UCS2(remaining, 5, 0, first); /* Entry 0-4 (5 chars) */
		FILL_UCS2(remaining, 6, 5, second); /* entry 5-10 (6 chars) */
		FILL_UCS2(remaining, 2, 11, third); /* Entry 11-12 (2 chars)*/
		filename += (remaining > 13 ? 13 : remaining);
		lfn[*entry_count].order &= ~ATTR_LFN_LAST_ENTRY;
		if (!*filename) {
			lfn[*entry_count].order |= ATTR_LFN_LAST_ENTRY;
		}
		(*entry_count)++;
		if (*entry_count == 64) {
			return;
		}
	}
}

void parse_short_name(directory_entry_t* entry, char* name, char* dotless)
{
	strlcpy(name, entry->name, 9);
	char* trans;
	for (trans = name; *trans; ++trans)
		if (*trans == ' ')
			*trans = 0;
	strlcat(name, ".", 10);
	strlcat(name, &(entry->name[8]), 13);
	for (trans = name; *trans; ++trans)
		if (*trans == ' ')
			*trans = 0;

	size_t namelen = strlen(name);		
	// remove trailing dot on dir names
	if (name[namelen - 1] == '.')
		name[namelen - 1] = 0;

	strlcpy(dotless, entry->name, 12);
	for (trans = dotless + 11; trans >= dotless; --trans)
		if (*trans == ' ')
			*trans = 0;
	dotless[12] = 0;
	name[12] = 0;
}

fs_directory_entry_t* parse_fat32_directory(fs_tree_t* tree, fat32_t* info, uint32_t cluster)
{
	unsigned char* buffer = kmalloc(info->clustersize);
	fs_directory_entry_t* list = NULL;
	lfn_t lfns[256] = { 0 };
	int16_t highest_lfn_order = -1;

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			dprintf("Read failure in parse_fat32_directory cluster=%08x\n", cluster);
			kfree(buffer);
			return NULL;
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

					if (entry->attr & ATTR_VOLUME_ID && entry->attr & ATTR_ARCHIVE && info->volume_name == NULL) {
						info->volume_name = strdup(dotless);
					} else {

						if (entry->attr == ATTR_LONG_NAME) {
							lfn_t* lfn = (lfn_t*)entry;
							memcpy(&lfns[lfn->order], (lfn_t*)entry, sizeof(lfn_t));
							if (lfn->order > highest_lfn_order) {
								highest_lfn_order = lfn->order;
							}
						} else {
							fs_directory_entry_t* file = kmalloc(sizeof(fs_directory_entry_t));
							if (highest_lfn_order > -1) {
								char longname[14 * (highest_lfn_order + 1)];
								char* nameptr = longname;
								for (int i = 0; i <= highest_lfn_order; ++i) {
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
								memset(&lfns, 0, sizeof(lfn_t) * 256);
							} else {
								file->filename = strdup(name);
							}
							file->alt_filename = strdup(name);
							file->lbapos = (uint64_t)(((uint64_t)entry->first_cluster_hi << 16) | (uint64_t)entry->first_cluster_lo);
							file->directory = tree;
							file->flags = 0;
							file->size = entry->size;

							if (entry->attr & ATTR_DIRECTORY) {
								file->flags |= FS_DIRECTORY;
							}
		
							file->next = list;
							list = file;
						}
					}
				}
			}
			bufferoffset += 32;
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}
		if (highest_lfn_order > -1) {
			dprintf("Cluster ended without sfn\n");
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			break;
		} else {
			cluster = nextcluster;
		}
	}
	kfree(buffer);

	return list;
}

bool fat32_read_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint32_t clustercount = 0;
	uint32_t first = 1;
	uint32_t amount_read = 0;
	unsigned char* clbuf = kmalloc(info->clustersize);

	// Advance to start when start != 0, for fseek()
	while ((clustercount++ < start / info->clustersize) && cluster < CLUSTER_BAD) {
		cluster = get_fat_entry(info, cluster);
	}

	while (true) {
		if (!read_cluster(info, cluster, clbuf)) {
			kprintf("Read failure in fat32_read_file cluster=%08x\n", cluster);
			kfree(clbuf);
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


	kfree(clbuf);

	return true;
}

bool fat32_write_file(void* f, uint64_t start, uint32_t length, unsigned char* buffer)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint64_t current_pos = 0;
	uint32_t first = 1;
	unsigned char* clbuf = kmalloc(info->clustersize);
	//memset(clbuf, 0, info->clustersize);

	if (start + length > file->size) {
		// File size must be extended to meet this requirement
		if (!fat32_extend_file(f, start + length - file->size)) {
			return false;
		}
	}

	while (cluster < CLUSTER_BAD && current_pos <= start + length) {
		if (!read_cluster(info, cluster, clbuf)) {
			kfree(clbuf);
			return false;
		}
		if (current_pos >= start && current_pos <= start + length) {
			int64_t to_write = length;
			if (length > info->clustersize) {
				to_write = info->clustersize;
			}
			if (first == 1) {
				/* Special case where start != 0 */
				memcpy(clbuf + (start % info->clustersize), buffer, to_write - (start % info->clustersize));
			} else {
				memcpy(clbuf, buffer, to_write);
			}
			if (!write_cluster(info, cluster, clbuf)) {
				dprintf("Write failure in fat32_write_file cluster=%08x\n", cluster);
				kfree(clbuf);
				return false;
			}
			buffer += to_write;
			first = 0;
		}
		cluster = get_fat_entry(info, cluster);
		current_pos += info->clustersize;
	}
	kfree(clbuf);
	return true;
}

bool fat32_truncate_file(void* f, size_t length)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	uint32_t cluster = file->lbapos;
	uint64_t current_pos = 0;
	bool first = true;
	unsigned char* clbuf = kmalloc(info->clustersize);
	//memset(clbuf, 0, info->clustersize);

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
	kfree(clbuf);

	/* Amend size in directory */
	uint8_t* buffer = kmalloc(info->clustersize); 
	cluster = file->directory->lbapos ? file->directory->lbapos : info->rootdircluster;

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			dprintf("couldnt read directory cluster %d in fat32_truncate_file\n", cluster_to_lba(info, cluster));
			kfree(buffer);
			return false;
		}
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster

		while (entries++ < info->clustersize / 32) {
			uint32_t this_file_cluster = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
			if (this_file_cluster == file->lbapos && entry->name[0] != 0 && !(entry->name[0] & 0x80) && entry->attr != ATTR_LONG_NAME) {
				entry->size = length;
				dump_hex(entry, 32);
				write_cluster(info, cluster, buffer);
				kfree(buffer);
				return true;
			}
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			dprintf("File not found in directory to amend size\n");
			kfree(buffer);
			return false;
		} else {
			cluster = nextcluster;
		}
	}
	kfree(buffer);
	return false;
}


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

	/* Calculate how many clusters are needed for this size of file */
	uint32_t size_in_clusters = size / info->clustersize;
	if (size_in_clusters == 0) {
		size_in_clusters = 1;
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
					kfree(blank_cluster);
					return false;
				}
			}
			if (last_cluster < CLUSTER_BAD) {
				set_fat_entry(info, last_cluster, cluster);
			}
			last_cluster = cluster;
			size_in_clusters--;
		}
		kfree(blank_cluster);
	}

	/* Amend the file's size in its directory entry */
	uint8_t* buffer = kmalloc(info->clustersize); 
	cluster = file->directory->lbapos ? file->directory->lbapos : info->rootdircluster;

	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			dprintf("couldnt read directory cluster %d in fat32_extend_file\n", cluster_to_lba(info, cluster));
			kfree(buffer);
			return false;
		}
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster

		while (entries++ < info->clustersize / 32) {
			uint32_t this_file_cluster = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
			if (this_file_cluster == file->lbapos && entry->name[0] != 0 && !(entry->name[0] & 0x80) && entry->attr != ATTR_LONG_NAME) {
				entry->size += size;
				dump_hex(entry, 32);
				write_cluster(info, cluster, buffer);
				kfree(buffer);
				return true;
			}
			bufferoffset += sizeof(directory_entry_t);
			entry = (directory_entry_t*)(buffer + bufferoffset);
		}

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= CLUSTER_BAD) {
			dprintf("File not found in directory to amend size\n");
			kfree(buffer);
			return false;
		} else {
			cluster = nextcluster;
		}
	}
	kfree(buffer);
	return false;
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
		dprintf("Read storage failed when extending directory\n");
		kfree(new_entries);
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
			dprintf("fat32: insert_entries_at() overlaps cluster edge, should not happen - LFN too long?\n");
			kfree(new_entries);
			return;
		}

	} while (entry_count > -1 && entries != info->clustersize / 32);

	if (entries < info->clustersize / 32) {
		memcpy(entry, short_entry, sizeof(directory_entry_t));
	}

	if (!write_cluster(info, cluster, buffer)) {
		dprintf("Write storage failed when extending directory\n");
	}

	kfree(new_entries);
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
			dprintf("File %s already exists in %s\n", name, treeitem->name);
			return 0;
		}
	}

	/* Calculate how many clusters are needed for this size of file */
	uint32_t size_in_clusters = size / info->clustersize;
	if (size_in_clusters == 0) {
		size_in_clusters = 1;
	}

	/* Allocate cluster chain to cover file length */
	uint32_t last_cluster = CLUSTER_END;
	uint32_t first_allocated_cluster = CLUSTER_END;
	uint8_t* blank_cluster = kmalloc(info->clustersize);
	/* Initialise the clusters to full of nulls */
	memset(blank_cluster, 0, info->clustersize);
	amend_free_count(info, -size_in_clusters);
	while (size_in_clusters > 0) {
		uint32_t cluster = find_next_free_fat_entry(info);
		if (cluster != CLUSTER_END) {
			set_fat_entry(info, cluster, CLUSTER_END);
			if (first_allocated_cluster == CLUSTER_END) {
				first_allocated_cluster = cluster;
			}
			/* Zero the clusters as we allocate them to the new file */
			if (!write_cluster(info, cluster, blank_cluster)) {
				kfree(blank_cluster);
				return 0;
			}
		}
		if (last_cluster != CLUSTER_END) {
			set_fat_entry(info, last_cluster, cluster);
		}
		last_cluster = cluster;
		size_in_clusters--;
	}
	kfree(blank_cluster);

	/* Add directory entry (including lfn) */
	uint8_t* buffer = kmalloc(info->clustersize);
	uint32_t cluster = dir_cluster;
	size_t entry_count = 0;
	directory_entry_t* new_entries = NULL;
	directory_entry_t short_entry = { 0 };

	build_lfn_chain(name, parsed_dir, &short_entry, &new_entries, &entry_count);
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
			kfree(buffer);
			return 0;
		}
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);
		uint32_t entries = 0; // max of info->clustersize / 32 file entries per cluster

		while (entries++ < info->clustersize / 32) {
			/* Hunt for a space with entry_count+1 contiguous empty dir entries */
			if ((entry->name[0] & 0x80) || entry->name[0] == 0) {
				if (start_of_space_bufferoffset == -1) {
					start_of_space_bufferoffset = bufferoffset;
					start_of_space_cluster = cluster;
					space_size = 0;
				}
				space_size++;
				if (space_size > entry_count + 1) {
					insert_entries_at(false, info, entries - 1, start_of_space_cluster, buffer, start_of_space_bufferoffset, &short_entry, new_entries, entry_count);
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
			return first_allocated_cluster;
		} else {
			cluster = nextcluster;
		}
	}
	kfree(buffer);
	return 0;	
}

uint64_t fat32_create_file(void* dir, const char* name, size_t size)
{
	return fat32_internal_create_file(dir, name, size, 0);
}

uint64_t fat32_create_directory(void* dir, const char* name)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t parent_dir_cluster = treeitem->lbapos ? treeitem->lbapos : info->rootdircluster;
	uint8_t* buffer = kmalloc(info->clustersize);
	int bufferoffset = 0;
	directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);

	uint64_t cluster = fat32_internal_create_file(dir, name, 0, ATTR_DIRECTORY);
	if (cluster == 0) {
		return 0;
	}

	if (!read_cluster(info, cluster, buffer)) {
		kfree(buffer);
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
		kfree(buffer);
		return 0;
	}

	kfree(buffer);
	return cluster;
}

void* fat32_get_directory(void* t)
{
	fs_tree_t* treeitem = (fs_tree_t*)t;
	if (treeitem) {
		fat32_t* info = (fat32_t*)treeitem->opaque;
		return (void*)parse_fat32_directory(treeitem, info, treeitem->lbapos ? treeitem->lbapos : info->rootdircluster);
	} else {
		kprintf("*** BUG *** fat32_get_directory: null fs_tree_t*!\n");
		return NULL;
	}
}

bool fat32_unlink_file(void* dir, const char* name)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fs_directory_entry_t* iter = NULL;
	uint32_t cluster = CLUSTER_END;
	uint32_t file_start = CLUSTER_END;
	int32_t freed = 0;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t dir_cluster = treeitem->lbapos ? treeitem->lbapos : info->rootdircluster;

	iter = parse_fat32_directory(treeitem, info, dir_cluster);
	for (; iter; iter = iter->next) {
		if (!strcmp(iter->filename, name)) {
			file_start = cluster = iter->lbapos;
			break;
		}
	}

	if (file_start == CLUSTER_END) {
		dprintf("not found in %s: %s\n", treeitem->name, name);
		return false;
	}

	/* Clear out the clusters to CLUSTER_FREE */
	do {
		uint32_t cur = cluster;
		cluster = get_fat_entry(info, cluster);
		freed++;
		if (cur < CLUSTER_BAD) {
			if (!set_fat_entry(info, cur, CLUSTER_FREE)) {
				return false;
			}
		}
	} while (cluster < CLUSTER_BAD);

	amend_free_count(info, freed);

	/* Remove related entries from directory */
	uint8_t* buffer = kmalloc(info->clustersize);
	directory_entry_t* entry_lfn_start = NULL;
	cluster = dir_cluster;
	while (true) {
		int bufferoffset = 0;
		if (!read_cluster(info, cluster, buffer)) {
			kprintf("Read failure in fat32_unlink_file cluster=%08x\n", cluster);
			kfree(buffer);
			return NULL;
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
							while (entry_lfn_start < entry) {
								*(entry_lfn_start->name) = DELETED_ENTRY;
								entry_lfn_start += sizeof(directory_entry_t);					
							}
							write_cluster(info, cluster, buffer);
							kfree(buffer);
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
	kfree(buffer);
	kprintf("fat32 unlink done\n");
	return true;
}

bool fat32_unlink_dir(void* dir, const char* name)
{
	fs_tree_t* treeitem = (fs_tree_t*)dir;
	fs_directory_entry_t* iter = NULL;
	fat32_t* info = (fat32_t*)treeitem->opaque;
	uint32_t dir_cluster = treeitem->lbapos;

	iter = parse_fat32_directory(treeitem, info, dir_cluster);
	if (iter) {
		/* Directory not empty */
		dprintf("fat32_unlink_dir: Directory not empty\n");
		return false;
	}
	return fat32_unlink_file(dir, name);
}

bool set_fat_entry(fat32_t* info, uint32_t cluster, uint32_t value)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint32_t* buffer = kmalloc(sd->block_size);
	uint32_t fat_entry_sector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t fat_entry_offset = (uint32_t) (cluster % (sd->block_size / 4));

	if (!read_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		dprintf("Read failure in set_fat_entry cluster=%08x\n", cluster);
		kfree(buffer);
		return false;
	}
	buffer[fat_entry_offset] = value & 0x0FFFFFFF;
	if (!write_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		dprintf("Write failure in set_fat_entry cluster=%08x to %08x\n", cluster, value);
		kfree(buffer);
		return false;
	}

	kfree(buffer);
	return true;
}

void amend_free_count(fat32_t* info, int adjustment)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	info->info->freecount += adjustment;
	if (info->info->freecount > info->length / info->clustersize) {
		info->info->freecount = info->length / info->clustersize;
	}
	write_storage_device(info->device_name, info->start + info->fsinfocluster, sd->block_size, (unsigned char*)info->info);
}


uint32_t find_next_free_fat_entry(fat32_t* info)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint32_t* buffer = kmalloc(sd->block_size);
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t fat_entry_sector = info->start + info->reservedsectors;

	while (offset < info->fatsize) {
		if (!read_storage_device(info->device_name, fat_entry_sector + offset, sd->block_size, (uint8_t*)buffer)) {
			kfree(buffer);
			return CLUSTER_END;
		}
		for (size_t t = 0; t < sd->block_size / sizeof(uint32_t); ++t) {
			if (buffer[t] == CLUSTER_FREE) {
				return count & 0x0FFFFFFF;
			}
			++count;
		}
		++offset;
	}
	kfree(buffer);
	return CLUSTER_END;
}

uint32_t get_fat_entry(fat32_t* info, uint32_t cluster)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint32_t* buffer = kmalloc(sd->block_size);
	uint32_t fat_entry_sector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t fat_entry_offset = (uint32_t) (cluster % (sd->block_size / 4));

	if (!read_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		kprintf("Read failure in get_fat_entry cluster=%08x\n", cluster);
		kfree(buffer);
		return 0x0fffffff;
	}
	uint32_t entry = buffer[fat_entry_offset] & 0x0FFFFFFF;
	kfree(buffer);
	return entry;
}

uint64_t cluster_to_lba(fat32_t* info, uint32_t cluster)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint64_t FirstDataSector = info->reservedsectors + (info->numberoffats * info->fatsize);
	uint64_t FirstSectorofCluster = ((cluster - 2) * (info->clustersize / sd->block_size) ) + FirstDataSector;
	return info->start + FirstSectorofCluster;
}

int read_fs_info(fat32_t* info)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!read_storage_device(info->device_name, info->start + info->fsinfocluster, sd->block_size, (unsigned char*)info->info)) {
		kprintf("Read failure in read_fs_info\n");
		return 0;
	}

	if (info->info->signature1 != FAT32_SIGNATURE || info->info->structsig != FAT32_SIGNATURE2 || info->info->trailsig != FAT32_SIGNATURE3) {
		kprintf("Malformed FAT32 info sector!\n");
		return 0;
	}

	return 1;
}

int read_fat(fat32_t* info)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	unsigned char* buffer = kmalloc(sd->block_size);
	_memset(buffer, 0, sd->block_size);
	if (!read_storage_device(info->device_name, info->start, sd->block_size, buffer)) {
		kprintf("FAT32: Could not read FAT parameter block!\n");
		kfree(buffer);
		return 0;
	}

	parameter_block_t* par = (parameter_block_t*)buffer;
	if (par->signature != 0x28 && par->signature != 0x29) {
		kprintf("FAT32: Invalid extended bios paramter block signature\n");
		kfree(buffer);
		return 0;
	}

	info->volume_name = NULL;
	info->rootdircluster = par->rootdircluster;
	info->reservedsectors = par->reservedsectors;
	info->fsinfocluster = par->fsinfocluster;
	info->numberoffats = par->numberoffats;
	info->fatsize = par->sectorsperfat;
	info->info = kmalloc(sizeof(fat32_fs_info_t));
	info->clustersize = par->sectorspercluster;
	info->clustersize *= sd->block_size;

	read_fs_info(info);
	info->root = parse_fat32_directory(NULL, info, info->rootdircluster);
	kfree(buffer);
	return 1;
}

fat32_t* fat32_mount_volume(const char* device_name)
{
	char found_guid[64];
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd) {
		return NULL;
	}
	fat32_t* info = kmalloc(sizeof(fat32_t));
	strlcpy(info->device_name, device_name, 16);

	uint64_t start = 0, length = 0;

	int success = 0;
	if (
		/* EFI system partition */
		!find_partition_of_type(device_name, 0x0B, found_guid, GPT_EFI_SYSTEM, &info->partitionid, &start, &length) &&
		!find_partition_of_type(device_name, 0x0C, found_guid, GPT_EFI_SYSTEM, &info->partitionid, &start, &length) &&
		/* Microsoft basic data partition */
		!find_partition_of_type(device_name, 0x0B, found_guid, GPT_MICROSOFT_BASIC_DATA, &info->partitionid, &start, &length) &&
		!find_partition_of_type(device_name, 0x0C, found_guid, GPT_MICROSOFT_BASIC_DATA, &info->partitionid, &start, &length)
	) {
		/* No partition found, attempt to mount the volume as whole disk */
		info->start = 0;
		info->partitionid = 0;
		info->length = sd->size * sd->block_size;
		success = read_fat(info);
		kprintf("Found FAT32 volume, device %s\n", device_name);
	} else {
		info->start = start;
		info->length = length * sd->block_size;
		if (info->partitionid != 0xFF) {
			kprintf("Found FAT32 partition, device %s, MBR partition %d\n", device_name, info->partitionid + 1);
		} else {
			kprintf("Found FAT32 partition, device %s, GPT partition %s\n", device_name, found_guid);
		}
		success = read_fat(info);
	}
	if (!success) {
		kfree(info);
		return NULL;
	}
	return info;
}

int fat32_attach(const char* device_name, const char* path)
{
	fat32_t* fat32fs = fat32_mount_volume(device_name);
	if (fat32fs) {
		return attach_filesystem(path, fat32_fs, fat32fs);
	}
	return 0;
}

void init_fat32()
{
	fat32_fs = kmalloc(sizeof(filesystem_t));
	strlcpy(fat32_fs->name, "fat32", 31);
	fat32_fs->mount = fat32_attach;
	fat32_fs->getdir = fat32_get_directory;
	fat32_fs->readfile = fat32_read_file;
	fat32_fs->createfile = fat32_create_file;
	fat32_fs->truncatefile = fat32_truncate_file;
	fat32_fs->createdir = fat32_create_directory;
	fat32_fs->writefile = fat32_write_file;
	fat32_fs->rm = fat32_unlink_file;
	fat32_fs->rmdir = fat32_unlink_dir;
	register_filesystem(fat32_fs);
}

