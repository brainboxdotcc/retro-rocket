#include <kernel.h>

static filesystem_t* fat32_fs = NULL;

uint64_t cluster_to_lba(fat32* info, uint32_t cluster);
uint32_t get_fat_entry(fat32* info, uint32_t cluster);
int fat32_read_file(void* file, uint32_t start, uint32_t length, unsigned char* buffer);
int fat32_attach(const char* device_name, const char* path);

fs_directory_entry_t* parse_fat32_directory(fs_tree_t* tree, fat32* info, uint32_t cluster)
{
	//kprintf("Cluster at start of fn: %d\n", cluster);

	unsigned char* buffer = (unsigned char*)kmalloc(info->clustersize);
	fs_directory_entry_t* list = NULL;

	while (1)
	{
		int bufferoffset = 0;
		//kprintf("Cluster at start of loop: %d\n", cluster);
		if (!read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, buffer))
		{
			kprintf("Read failure in parse_fat32_directory cluster=%08x\n", cluster);
			kfree(buffer);
			return NULL;
		}
		
		DirectoryEntry* entry = (DirectoryEntry*)(buffer + bufferoffset);

		//dump_hex(entry, 32);

		int entries = 0; // max of 128 file entries per cluster

		while (entry->name[0] != 0 && entries++ < 128)
		{
			if (entry->name[0] != 0xE5 && entry->name[0] != 0)
			{
				char name[13];
				char dotless[13];

				strlcpy(name, entry->name, 9);
				//kprintf("1 '%s'\n", name);
				char* trans;
				for (trans = name; *trans; ++trans)
					if (*trans == ' ')
						*trans = 0;
				//kprintf("2 '%s'\n", name);
				strlcat(name, ".", 10);
				//kprintf("3 '%s'\n", name);
				strlcat(name, &(entry->name[8]), 13);
				//kprintf("4 '%s'\n", name);
				for (trans = name; *trans; ++trans)
					if (*trans == ' ')
						*trans = 0;
				
				// remove trailing oot on dir names
				if (name[strlen(name) - 1] == '.')
					name[strlen(name) - 1] = 0;

				strlcpy(dotless, entry->name, 12);
				for (trans = dotless + 11; trans >= dotless; --trans)
					if (*trans == ' ')
						*trans = 0;
				dotless[12] = 0;
				name[12] = 0;

				if (name[0] == 0)
					break;

				if (name[0] != '.')
				{


					if (entry->attr & ATTR_VOLUME_ID && entry->attr & ATTR_ARCHIVE && tree == NULL && info->volume_name == NULL)
					{
						info->volume_name = strdup(dotless);
						//kprintf("FAT32 volume label: '%s'\n", info->volume_name);
					}
					else
					{
	
						fs_directory_entry_t* file = (fs_directory_entry_t*)kmalloc(sizeof(fs_directory_entry_t));
	
						//kprintf("5 '%s'\n", name);
						file->filename = strdup(name);
						file->lbapos = (uint32_t)(((uint32_t)entry->first_cluster_hi << 16) | (uint32_t)entry->first_cluster_lo);
						file->directory = tree;
						file->flags = 0;
						file->size = entry->size;
	
						//kprintf("%04x %04x\n", entry->create_time, entry->create_date);
	
						if (entry->attr & ATTR_DIRECTORY)
							file->flags |= FS_DIRECTORY;
	
	
						// XXX
						//kprintf("%d. '%s' flags=%02x size=%d clus=%08x\n", entries, file->filename, file->flags, file->size, file->lbapos);
	
						//if (file->size > 10000000)
						//	for(;;);
	
						file->next = list;
						list = file;
					}
				}
			}
			bufferoffset += 32;
			entry = (DirectoryEntry*)(buffer + bufferoffset);
		}

		if (entry->name[0] == 0)
			break;

		//kprintf("End dir parse\n");

		// advnce to next cluster in chain until EOF
		uint32_t nextcluster = get_fat_entry(info, cluster);
		if (nextcluster >= 0x0ffffff0)
		{
			//kprintf("Cluster break\n");
			break;
		}
		else
		{
			//kprintf("Old cluster=%d\n", cluster);
			cluster = nextcluster;
			//kprintf("Next cluster=%d\n", cluster);
		}
	}
	kfree(buffer);

	return list;
}

int fat32_read_file(void* f, uint32_t start, uint32_t length, unsigned char* buffer)
{
	fs_directory_entry_t* file = (fs_directory_entry_t*)f;
	fs_tree_t* tree = (fs_tree_t*)file->directory;
	fat32* info = (fat32*)tree->opaque;

	//kprintf("fat32_read_file start=%d len=%d\n", start, length);

	uint32_t cluster = file->lbapos;
	uint32_t clustercount = 0;
	uint32_t first = 1;
	unsigned char* clbuf = (unsigned char*)kmalloc(info->clustersize);

	//kprintf("First cluster: %08x\n", cluster);

	// vAdvance until we are at the correct location
	while ((clustercount++ < start / info->clustersize) && (cluster < 0x0ffffff0))
	{
		cluster = get_fat_entry(info, cluster);
		//kprintf("Advance to next cluster %08x\n", cluster);
	}

	while (1)
	{
		//kprintf("Read file clusters cluster=%08x\n", cluster);
		if (!read_storage_device(info->device_name, cluster_to_lba(info, cluster), info->clustersize, clbuf))
		{
			kprintf("Read failure in fat32_read_file cluster=%08x\n", cluster);
			kfree(clbuf);
			return 0;
		}

		//dump_hex(buffer, 16);

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
	if (treeitem)
	{
		fat32* info = (fat32*)treeitem->opaque;
		//kprintf("Asked for fat32 dir '%s' pos=%d\n", treeitem->name, treeitem->lbapos);
		return (void*)parse_fat32_directory(treeitem, info, treeitem->lbapos ? treeitem->lbapos : info->rootdircluster);
	}
	else
	{
		kprintf("*** BUG *** fat32_get_directory: null fs_tree_t*!\n");
		return NULL;
	}
}

uint32_t get_fat_entry(fat32* info, uint32_t cluster)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint32_t* buffer = (uint32_t*)kmalloc(sd->block_size);
	uint32_t FATEntrySector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t FATEntryOffset = (uint32_t) (cluster % (sd->block_size / 4));  // 512/4=128

	//kprintf("cluster=%08x fatentrysector=%08x fatentryoffset=%08x\n", cluster, FATEntrySector, FATEntryOffset);

	if (!read_storage_device(info->device_name, FATEntrySector, sd->block_size, (unsigned char*)buffer))
	{
		kprintf("Read failure in get_fat_entry cluster=%08x\n", cluster);
		return 0x0fffffff;
	}
	//dump_hex(buffer, 16);
	//kprintf("Sector at LBA %08x offset %08x\n", ThisFATSecNum, ThisFATEntOffset);
	uint32_t entry = buffer[FATEntryOffset] & 0x0FFFFFFF;
	kfree(buffer);
	return entry;
}

uint64_t cluster_to_lba(fat32* info, uint32_t cluster)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	uint64_t FirstDataSector = info->reservedsectors + (info->numberoffats * info->fatsize);
	uint64_t FirstSectorofCluster = ((cluster - 2) * (info->clustersize / sd->block_size) ) + FirstDataSector;
	return info->start + FirstSectorofCluster;
}

int read_fs_info(fat32* info)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!read_storage_device(info->device_name, info->start + info->fsinfocluster, sd->block_size, (unsigned char*)info->info))
	{
		kprintf("Read failure in read_fs_info\n");
		return 0;
	}

	if (info->info->signature1 != 0x41615252)
	{
		kprintf("Malformed FAT32 FSInfo sector!\n");
		// TODO: Mount readonly here!!!
		return 0;
	}

	return 1;
}

int read_fat(fat32* info)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	unsigned char* buffer = (unsigned char*)kmalloc(sd->block_size);
	_memset(buffer, 0, sd->block_size);
	if (!read_storage_device(info->device_name, info->start, sd->block_size, buffer))
	{
		kprintf("FAT32: Could not read partition boot sector!\n");
		kfree(buffer);
		return 0;
	}

	ParameterBlock* par = (ParameterBlock*)buffer;

	if (par->signature != 0x28 && par->signature != 0x29)
	{
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
	info->info = (FSInfo*)kmalloc(sizeof(FSInfo));

	info->clustersize = par->sectorspercluster;
	info->clustersize *= sd->block_size;

	//kprintf("FAT32: Cluster size: %d (%d sectors)\n", info->clustersize, info->clustersize / 512);

	read_fs_info(info);

	info->root = parse_fat32_directory(NULL, info, info->rootdircluster);

	//uint32_t pos = (uint32_t)(((uint32_t)de->first_cluster_hi << 16) + (uint32_t)de->first_cluster_lo);
	
	//int j;
	//for (j = 0; j < 18; j++)
	//	kprintf("%d %08x\n", j, get_fat_entry(info, j));

	kfree(buffer);

	return 1;
}

fat32* fat32_mount_volume(const char* device_name)
{
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd) {
		return NULL;
	}
	unsigned char* buffer = (unsigned char*)kmalloc(sd->block_size);
	fat32* info = (fat32*)kmalloc(sizeof(fat32));
	_memset(buffer, 0, sd->block_size);
	strlcpy(info->device_name, device_name, 16);
	if (!read_storage_device(device_name, 0, sd->block_size, buffer))
	{
		kprintf("FAT32: Could not read partition table sector!\n");
		kfree(info);
		return NULL;
	}

	PartitionTable* ptab = (PartitionTable*)(buffer + PARTITION_TABLE_OFFSET);

	//dump_hex(ptab, sizeof(ptab));

	int i, success;
	for (i = 0; i < 4; i++)
	{
		//kprintf("%02x %08x %08x\n", ptab->p_entry[i].systemid, ptab->p_entry[i].startlba, ptab->p_entry[i].length);
		Partition* p = &(ptab->p_entry[i]);
		if (p->systemid == 0x0B || p->systemid == 0x0C)
		{
			kprintf("Found FAT32 partition, device %s, partition %d\n", device_name, i + 1);
			info->partitionid = i;
			info->start = p->startlba;
			info->length = p->length;
			success = read_fat(info);
			kfree(buffer);
			return success ? info : NULL;
		}
	}

	kprintf("No FAT32 partitions found on device '%s'\n", device_name);
	kfree(buffer);
	return NULL;
}

int fat32_attach(const char* device_name, const char* path)
{
	fat32* fat32fs = fat32_mount_volume(device_name);
	if (fat32fs) {
		return attach_filesystem(path, fat32_fs, fat32fs);
	}
	return 0;
}

void init_fat32()
{
	fat32_fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
	strlcpy(fat32_fs->name, "fat32", 31);
	fat32_fs->mount = fat32_attach;
	fat32_fs->getdir = fat32_get_directory;
	fat32_fs->readfile = fat32_read_file;
	fat32_fs->writefile = NULL;
	fat32_fs->rm = NULL;
	register_filesystem(fat32_fs);
}

