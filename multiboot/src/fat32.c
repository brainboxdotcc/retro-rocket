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

FS_DirectoryEntry* ParseFat32Directory(FS_Tree* node, fat32* info, u32int start_lba, u32int lengthbytes)
{
	/*if (!ide_read_sectors(info->drivenumber, lengthbytes / 2048, start_lba, (unsigned int)dirbuffer))
	{
		kprintf("ISO9660: Could not read LBA sectors 0x%x+0x%x when loading directory!\n", start_lba, lengthbytes / 2048);
		kfree(dirbuffer);
		return NULL;
	}*/
	return NULL;
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

void ReadFAT(fat32* info)
{
	kprintf("Parsing FAT32 on drive %d starting at LBA %08x\n", info->drivenumber, info->start);

	unsigned char* buffer = (unsigned char*)kmalloc(512);
	_memset(buffer, 0, 512);
	if (!ide_read_sectors(info->drivenumber, 1, info->start, (unsigned int)buffer))
	{
		kprintf("FAT32: Could not read partition boot sector!\n");
		kfree(buffer);
		return;
	}

	ParameterBlock* par = (ParameterBlock*)buffer;
	//DumpHex(par, sizeof(ParameterBlock));

	if (par->signature != 0x28 && par->signature != 0x29)
	{
		kprintf("FAT32: Invalid extended bios paramter block signature\n");
		kfree(buffer);
		return;
	}

	*(par->volumelabel + 11) = 0;
	while (par->volumelabel[strlen(par->volumelabel) - 1] == ' ')
		par->volumelabel[strlen(par->volumelabel) - 1] = 0;

	info->volume_name = strdup(par->volumelabel);
	//kprintf("'%s'\n", par->volumelabel);

	kfree(buffer);
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

	int i;
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
			ReadFAT(info);
			kfree(buffer);
			return info;
		}
	}

	//kprintf("fat32: Mounted volume '%s' on drive %d\n", info->volume_name, drivenumber);
	kprintf("No FAT32 partitions found on IDE drive %d\n", drivenumber);
	kfree(buffer);
	return info;
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
		//attach_filesystem(path, fat32_fs, fat32fs);
	}
}

