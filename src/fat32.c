#include <kernel.h>

static filesystem_t* fat32_fs = NULL;

uint64_t cluster_to_lba(fat32_t* info, uint32_t cluster);
uint32_t get_fat_entry(fat32_t* info, uint32_t cluster);
int fat32_read_file(void* file, uint32_t start, uint32_t length, unsigned char* buffer);
int fat32_attach(const char* device_name, const char* path);

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

fs_directory_entry_t* parse_fat32_directory(fs_tree_t* tree, fat32_t* info, uint32_t cluster)
{
	//kprintf("Cluster at start of fn: %d\n", cluster);

	unsigned char* buffer = kmalloc(info->clustersize);
	fs_directory_entry_t* list = NULL;
	lfn_t lfns[256] = { 0 };
	int16_t highest_lfn_order = -1;

	while (true) {
		int bufferoffset = 0;
		//kprintf("Cluster at start of loop: %d\n", cluster);
		if (!read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer)) {
			kprintf("Read failure in parse_fat32_directory cluster=%08x\n", cluster);
			kfree(buffer);
			return NULL;
		}
		
		directory_entry_t* entry = (directory_entry_t*)(buffer + bufferoffset);

		int entries = 0; // max of 128 file entries per cluster

		while (entry->name[0] != 0 && entries++ < 128) {
			if (!(entry->name[0] & 0x80) && entry->name[0] != 0) {
				char name[13];
				char dotless[13];

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

				if (name[0] == 0)
					break;

				if (name[0] != '.') {

					dprintf("Entry: %s\n", name);

					if (entry->attr & ATTR_VOLUME_ID && entry->attr & ATTR_ARCHIVE && info->volume_name == NULL) {
						info->volume_name = strdup(dotless);
					} else {

						if (entry->attr == ATTR_LONG_NAME) {
							dprintf("Long name\n");
							lfn_t* lfn = (lfn_t*)entry;
							memcpy(&lfns[lfn->order], (lfn_t*)entry, sizeof(lfn_t));
							if (lfn->order > highest_lfn_order) {
								highest_lfn_order = lfn->order;
							}
						} else {
							dprintf("Short name\n");
							fs_directory_entry_t* file = kmalloc(sizeof(fs_directory_entry_t));
							if (highest_lfn_order > -1) {
								char longname[14 * (highest_lfn_order + 1)];
								char* nameptr = longname;
								for (int i = 0; i <= highest_lfn_order; ++i) {
									if (lfns[i].first[0] == 0 || lfns[i].first[0] == 0xffff)
										continue;
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
							file->lbapos = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
							file->directory = tree;
							file->flags = 0;
							file->size = entry->size;

							if (entry->attr & ATTR_DIRECTORY)
								file->flags |= FS_DIRECTORY;
		
							// XXX
							// dprintf("%d. '%s' flags=%02x size=%d clus=%08x\n", entries, file->filename, file->flags, file->size, file->lbapos);

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

		if (entry->name[0] == 0)
			break;

		// advance to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= 0x0ffffff0) {
			break;
		} else {
			cluster = nextcluster;
		}
	}
	kfree(buffer);

	return list;
}

int fat32_read_file(void* f, uint32_t start, uint32_t length, unsigned char* buffer)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32_t* info = (fat32_t*)tree->opaque;

	//kprintf("fat32_read_file start=%d len=%d\n", start, length);

	uint32_t cluster = file->lbapos;
	uint32_t clustercount = 0;
	uint32_t first = 1;
	unsigned char* clbuf = kmalloc(info->clustersize);

	//kprintf("First cluster: %08x\n", cluster);

	// vAdvance until we are at the correct location
	while ((clustercount++ < start / info->clustersize) && (cluster < 0x0ffffff0)) {
		cluster = get_fat_entry(info, cluster);
		//kprintf("Advance to next cluster %08x\n", cluster);
	}

	while (true) {
		//kprintf("Read file clusters cluster=%08x\n", cluster);
		if (!read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, clbuf)) {
			kprintf("Read failure in fat32_read_file cluster=%08x\n", cluster);
			kfree(clbuf);
			return 0;
		}

		int to_read = length - start;
		if (length > info->clustersize)
			to_read = info->clustersize;
		if (first == 1)
			memcpy(buffer, clbuf + (start % info->clustersize), to_read - (start % info->clustersize));
		else
			memcpy(buffer, clbuf, to_read);

		buffer += to_read;
		first = 0;

		cluster = get_fat_entry(info, cluster);

		if (cluster >= 0x0ffffff0)
			break;
	}


	kfree(clbuf);

	return 1;
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

uint32_t get_fat_entry(fat32_t* info, uint32_t cluster)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint32_t* buffer = kmalloc(sd->block_size);
	uint32_t FATEntrySector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t FATEntryOffset = (uint32_t) (cluster % (sd->block_size / 4));  // 512/4=128

	if (!read_storage_device(info->device_name, FATEntrySector, sd->block_size, (unsigned char*)buffer)) {
		kprintf("Read failure in get_fat_entry cluster=%08x\n", cluster);
		return 0x0fffffff;
	}
	uint32_t entry = buffer[FATEntryOffset] & 0x0FFFFFFF;
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

	if (info->info->signature1 != FAT32_SIGNATURE) {
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
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd) {
		return NULL;
	}
	fat32_t* info = kmalloc(sizeof(fat32_t));
	strlcpy(info->device_name, device_name, 16);

	if (
		!find_partition_of_type(device_name, 0x0B, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", &info->partitionid, &info->start, &info->length) &&
		!find_partition_of_type(device_name, 0x0C, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", &info->partitionid, &info->start, &info->length)
	) {
		/* No partition found, attempt to mount the volume as whole disk */
		info->start = 0;
		info->partitionid = 0;
		info->length = sd->size * sd->block_size;
	} else {
		kprintf("Found FAT32 partition, device %s, partition %d\n", device_name, info->partitionid + 1);
	}
	int success = read_fat(info);
	return success ? info : NULL;
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
	fat32_fs->writefile = NULL;
	fat32_fs->rm = NULL;
	register_filesystem(fat32_fs);
}

