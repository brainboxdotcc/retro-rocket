#include "../include/kernel.h"
#include "../include/fat32.h"
#include "../include/kprintf.h"
#include "../include/io.h"
#include "../include/ata.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"
#include "../include/filesystem.h"
#include "../include/debugger.h"

static FS_FileSystem* fat32_fs = NULL;

extern ide_device ide_devices[4];

u32int ClusLBA(fat32* info, u32int cluster);

FS_DirectoryEntry* ParseFAT32Dir(FS_Tree* tree, fat32* info, u32int cluster)
{
	unsigned char* buffer = (unsigned char*)kmalloc(512 * 8);
	int bufferoffset = 0;
	FS_DirectoryEntry* list = NULL;
	u8int end = 0;
	while (!end)
	{
		end = 1;
		if (!ide_read_sectors(info->drivenumber, SECTORS_PER_CLUSTER, ClusLBA(info, cluster), (unsigned int)buffer))
		{
			kfree(buffer);
			return NULL;
		}
		
		DirectoryEntry* entry = (DirectoryEntry*)(buffer + bufferoffset);

		while (entry->name[0] != 0)
		{
			if (entry->name[0] != 0xE5)
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
				strlcat(name, &(entry->name[8]), 12);
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


				if (entry->attr & ATTR_VOLUME_ID && entry->attr & ATTR_ARCHIVE && tree == NULL)
				{
					kfree(info->volume_name);
					info->volume_name = strdup(dotless);
					kprintf("FAT32 volume label: '%s'\n", info->volume_name);
				}
				else
				{

					FS_DirectoryEntry* file = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));

					//kprintf("5 '%s'\n", name);
					file->filename = strdup(name);
					file->lbapos = (u32int)(((u32int)entry->first_cluster_hi << 16) | (u32int)entry->first_cluster_lo);
					file->directory = tree;
					file->flags = 0;
					file->size = entry->size;
					if (entry->attr & ATTR_DIRECTORY)
						file->flags |= FS_DIRECTORY;


					kprintf("'%s' flags=%02x size=%d clus=%08x\n", file->filename, file->flags, file->size, file->lbapos);

					if (list == NULL)
					{
						file->next = NULL;
						list = file;
					}
					else
					{
						file->next = list;
						list = file;
					}
				}
			}
			bufferoffset += 32;
			entry = (DirectoryEntry*)(buffer + bufferoffset);
		}
	}
	kfree(buffer);

	return list;
}

int fat32_read_file(void* f, u32int start, u32int length, unsigned char* buffer)
{
	FS_DirectoryEntry* file = (FS_DirectoryEntry*)f;
	FS_Tree* tree = (FS_Tree*)file->directory;
	fat32* info = (fat32*)tree->opaque;

	return 1;
}

void* fat32_get_directory(void* t)
{
	return NULL;
}

u32int GetFATEntry(fat32* info, u32int cluster)
{
	u32int* buffer = (u32int*)kmalloc(512);
	//u32int FATOffset = cluster * 4;
	//u32int ThisFATSecNum = info->start + info->reservedsectors + (FATOffset / 512);
	//u32int ThisFATEntOffset = FATOffset % 512;
	//
	u32int FATEntrySector = info->start + info->reservedsectors + ((cluster * 4) / 512);
	u32int FATEntryOffset = (u32int) (cluster % 512);

	kprintf("cluster=%08x fatentrysector=%08x fatentryoffset=%08x\n", cluster, FATEntrySector, FATEntryOffset);

	if (!ide_read_sectors(info->drivenumber, 1, FATEntrySector, (unsigned int)buffer))
	{
	}
	//DumpHex(buffer, 16);
	//kprintf("Sector at LBA %08x offset %08x\n", ThisFATSecNum, ThisFATEntOffset);
	u32int entry = buffer[FATEntryOffset] & 0x0FFFFFFF;
	kfree(buffer);
	return entry;
}

u32int ClusLBA(fat32* info, u32int cluster)
{
	u32int FirstDataSector = info->reservedsectors + (info->numberoffats * info->fatsize);
	u32int FirstSectorofCluster = ((cluster - 2) * 8) + FirstDataSector;
	return info->start + FirstSectorofCluster;
}

int ReadFSInfo(fat32* info)
{
	if (!ide_read_sectors(info->drivenumber, 1, info->start + info->fsinfocluster, (unsigned int)info->info))
	{
		return 0;
	}

	kprintf("sig1=%08x\n", info->info->signature1);

	DumpHex(info->info, 16);

	return 1;
}

int ReadFAT(fat32* info)
{
	//kprintf("Parsing FAT32 on drive %d starting at LBA %08x\n", info->drivenumber, info->start);

	unsigned char* buffer = (unsigned char*)kmalloc(512 * 8);
	_memset(buffer, 0, 512 * 8);
	if (!ide_read_sectors(info->drivenumber, 1, info->start, (unsigned int)buffer))
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

	*(par->volumelabel + 11) = 0;
	while (par->volumelabel[strlen(par->volumelabel) - 1] == ' ')
		par->volumelabel[strlen(par->volumelabel) - 1] = 0;

	info->volume_name = strdup(par->volumelabel);
	info->rootdircluster = par->rootdircluster;
	info->reservedsectors = par->reservedsectors;
	info->fsinfocluster = par->fsinfocluster;
	info->numberoffats = par->numberoffats;
	info->fatsize = par->sectorsperfat;
	info->info = (FSInfo*)kmalloc(sizeof(FSInfo));

	ReadFSInfo(info);

	if (par->sectorspercluster != 8)
	{
		kprintf("FAT32: Invalid sectors per cluster value (%d)\n", par->sectorspercluster);
		kfree(buffer);
		return 0;
	}

	info->root = ParseFAT32Dir(NULL, info, info->rootdircluster);

	//u32int pos = (u32int)(((u32int)de->first_cluster_hi << 16) + (u32int)de->first_cluster_lo);
	
	//int j;
	//for (j = 0; j < 18; j++)
	//	kprintf("%d %08x\n", j, GetFATEntry(info, j));

	kfree(buffer);

	return 1;
}

fat32* fat32_mount_volume(u32int drivenumber)
{
	unsigned char* buffer = (unsigned char*)kmalloc(512);
	fat32* info = (fat32*)kmalloc(sizeof(fat32));
	_memset(buffer, 0, 512);
	if (!ide_read_sectors(drivenumber, 1, 0, (unsigned int)buffer))
	{
		kprintf("FAT32: Could not partition table sector!\n");
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

void fat32_attach(u32int drivenumber, const char* path)
{
	if (drivenumber >= 0)
	{
		fat32* fat32fs = fat32_mount_volume(drivenumber);
		if (fat32fs)
		{
			//attach_filesystem(path, fat32_fs, fat32fs);
		}
	}
}

