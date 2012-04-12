#include <kernel.h>

static FS_FileSystem* fat32_fs = NULL;

extern ide_device ide_devices[4];

u64 ClusLBA(fat32* info, u32 cluster);
u32 GetFATEntry(fat32* info, u32 cluster);

FS_DirectoryEntry* ParseFAT32Dir(FS_Tree* tree, fat32* info, u32 cluster)
{
	//kprintf("Cluster at start of fn: %d\n", cluster);

	unsigned char* buffer = (unsigned char*)kmalloc(info->clustersize);
	FS_DirectoryEntry* list = NULL;

	while (1)
	{
		int bufferoffset = 0;
		//kprintf("Cluster at start of loop: %d\n", cluster);
		if (!ide_read_sectors(info->drivenumber, info->clustersize / 512, ClusLBA(info, cluster), (u64)buffer))
		{
			kprintf("Read failure in ParseFAT32Dir cluster=%08x\n", cluster);
			kfree(buffer);
			return NULL;
		}
		
		DirectoryEntry* entry = (DirectoryEntry*)(buffer + bufferoffset);

		//DumpHex(entry, 32);

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
						kprintf("FAT32 volume label: '%s'\n", info->volume_name);
					}
					else
					{
	
						FS_DirectoryEntry* file = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
	
						//kprintf("5 '%s'\n", name);
						file->filename = strdup(name);
						file->lbapos = (u32)(((u32)entry->first_cluster_hi << 16) | (u32)entry->first_cluster_lo);
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
		u32 nextcluster = GetFATEntry(info, cluster);
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

int fat32_read_file(void* f, u32 start, u32 length, unsigned char* buffer)
{
	FS_DirectoryEntry* file = (FS_DirectoryEntry*)f;
	FS_Tree* tree = (FS_Tree*)file->directory;
	fat32* info = (fat32*)tree->opaque;

	//kprintf("fat32_read_file start=%d len=%d\n", start, length);

	u32 cluster = file->lbapos;
	u32 clustercount = 0;
	u32 first = 1;
	unsigned char* clbuf = (unsigned char*)kmalloc(info->clustersize);

	//kprintf("First cluster: %08x\n", cluster);

	// vAdvance until we are at the correct location
	while ((clustercount++ < start / info->clustersize) && (cluster < 0x0ffffff0))
	{
		cluster = GetFATEntry(info, cluster);
		//kprintf("Advance to next cluster %08x\n", cluster);
	}

	while (1)
	{
		//kprintf("Read file clusters cluster=%08x\n", cluster);
		if (!ide_read_sectors(info->drivenumber, info->clustersize / 512, ClusLBA(info, cluster), (u64)clbuf))
		{
			kprintf("Read failure in fat32_read_file cluster=%08x\n", cluster);
			kfree(clbuf);
			return 0;
		}

		//DumpHex(buffer, 16);

		int to_read = length - start;
		if (length > info->clustersize)
			to_read = info->clustersize;
		if (first == 1)
			memcpy(buffer, clbuf + (start % info->clustersize), to_read - (start % info->clustersize));
		else
			memcpy(buffer, clbuf, to_read);

		buffer += to_read;
		first = 0;

		cluster = GetFATEntry(info, cluster);

		if (cluster >= 0x0ffffff0)
			break;
	}


	kfree(clbuf);

	return 1;
}

void* fat32_get_directory(void* t)
{
	FS_Tree* treeitem = (FS_Tree*)t;
	if (treeitem)
	{
		fat32* info = (fat32*)treeitem->opaque;
		//kprintf("Asked for fat32 dir '%s' pos=%d\n", treeitem->name, treeitem->lbapos);
		return (void*)ParseFAT32Dir(treeitem, info, treeitem->lbapos ? treeitem->lbapos : info->rootdircluster);
	}
	else
	{
		kprintf("*** BUG *** fat32_get_directory: null FS_Tree*!\n");
		return NULL;
	}
}

u32 GetFATEntry(fat32* info, u32 cluster)
{
	u32* buffer = (u32*)kmalloc(512);
	//u32 FATOffset = cluster * 4;
	//u32 ThisFATSecNum = info->start + info->reservedsectors + (FATOffset / 512);
	//u32 ThisFATEntOffset = FATOffset % 512;
	//
	u32 FATEntrySector = info->start + info->reservedsectors + ((cluster * 4) / 512);
	u32 FATEntryOffset = (u32) (cluster % 128);  // 512/4

	//kprintf("cluster=%08x fatentrysector=%08x fatentryoffset=%08x\n", cluster, FATEntrySector, FATEntryOffset);

	if (!ide_read_sectors(info->drivenumber, 1, FATEntrySector, (u64)buffer))
	{
		kprintf("Read failure in GetFATEntry cluster=%08x\n", cluster);
		return 0x0fffffff;
	}
	//DumpHex(buffer, 16);
	//kprintf("Sector at LBA %08x offset %08x\n", ThisFATSecNum, ThisFATEntOffset);
	u32 entry = buffer[FATEntryOffset] & 0x0FFFFFFF;
	kfree(buffer);
	return entry;
}

u64 ClusLBA(fat32* info, u32 cluster)
{
	u64 FirstDataSector = info->reservedsectors + (info->numberoffats * info->fatsize);
	u64 FirstSectorofCluster = ((cluster - 2) * (info->clustersize / 512) ) + FirstDataSector;
	return info->start + FirstSectorofCluster;
}

int ReadFSInfo(fat32* info)
{
	if (!ide_read_sectors(info->drivenumber, 1, info->start + info->fsinfocluster, (u64)info->info))
	{
		kprintf("Read failure in ReadFSInfo\n");
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

int ReadFAT(fat32* info)
{
	//kprintf("Parsing FAT32 on drive %d starting at LBA %08x\n", info->drivenumber, info->start);

	unsigned char* buffer = (unsigned char*)kmalloc(512);
	_memset(buffer, 0, 512);
	if (!ide_read_sectors(info->drivenumber, 1, info->start, (u64)buffer))
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
	info->clustersize *= 512;

	kprintf("FAT32: Cluster size: %d (%d sectors)\n", info->clustersize, info->clustersize / 512);

	ReadFSInfo(info);

	info->root = ParseFAT32Dir(NULL, info, info->rootdircluster);

	//u32 pos = (u32)(((u32)de->first_cluster_hi << 16) + (u32)de->first_cluster_lo);
	
	//int j;
	//for (j = 0; j < 18; j++)
	//	kprintf("%d %08x\n", j, GetFATEntry(info, j));

	kfree(buffer);

	return 1;
}

fat32* fat32_mount_volume(u32 drivenumber)
{
	unsigned char* buffer = (unsigned char*)kmalloc(512);
	fat32* info = (fat32*)kmalloc(sizeof(fat32));
	_memset(buffer, 0, 512);
	if (!ide_read_sectors(drivenumber, 1, 0, (u64)buffer))
	{
		kprintf("FAT32: Could not read partition table sector!\n");
		kfree(info);
		return NULL;
	}

	PartitionTable* ptab = (PartitionTable*)(buffer + PARTITION_TABLE_OFFSET);

	//DumpHex(ptab, sizeof(ptab));

	int i, success;
	for (i = 0; i < 4; i++)
	{
		//kprintf("%02x %08x %08x\n", ptab->p_entry[i].systemid, ptab->p_entry[i].startlba, ptab->p_entry[i].length);
		Partition* p = &(ptab->p_entry[i]);
		if (p->systemid == 0x0B || p->systemid == 0x0C)
		{
			kprintf("Found FAT32 partition, IDE drive %d, partition %d\n", drivenumber, i + 1);
			info->drivenumber = drivenumber;
			info->partitionid = i;
			info->start = p->startlba;
			info->length = p->length;
			success = ReadFAT(info);
			kfree(buffer);
			return success ? info : NULL;
		}
	}

	//kprintf("fat32: Mounted volume '%s' on drive %d\n", info->volume_name, drivenumber);
	kprintf("No FAT32 partitions found on IDE drive %d\n", drivenumber);
	kfree(buffer);
	return NULL;
}

void init_fat32()
{
	fat32_fs = (FS_FileSystem*)kmalloc(sizeof(FS_FileSystem));
	strlcpy(fat32_fs->name, "fat32", 31);
	fat32_fs->getdir = fat32_get_directory;
	fat32_fs->readfile = fat32_read_file;
	fat32_fs->writefile = NULL;
	fat32_fs->rm = NULL;
	register_filesystem(fat32_fs);
}

int find_first_harddisk()
{
	int i;
	for (i = 0; i < 4; i++)
		if (ide_devices[i].type != IDE_ATAPI)
			return i;
	return -1;
}

void fat32_attach(u32 drivenumber, const char* path)
{
	if (drivenumber >= 0)
	{
		fat32* fat32fs = fat32_mount_volume(drivenumber);
		if (fat32fs)
		{
			attach_filesystem(path, fat32_fs, fat32fs);
		}
	}
}

