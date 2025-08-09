#include "kernel.h"

static filesystem_t* fat32_fs = NULL;


int read_fs_info(fat32_t* info)
{
	if (!info) {
		return 0;
	}
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return 0;
	}
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
	if (!info) {
		return 0;
	}
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return 0;
	}
	unsigned char* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return 0;
	}
	memset(buffer, 0, sd->block_size);
	if (!read_storage_device(info->device_name, info->start, sd->block_size, buffer)) {
		kprintf("FAT32: Could not read FAT parameter block!\n");
		kfree_null(&buffer);
		return 0;
	}

	parameter_block_t* par = (parameter_block_t*)buffer;
	if (par->signature != 0x28 && par->signature != 0x29) {
		kprintf("FAT32: Invalid extended bios parameter block signature\n");
		kfree_null(&buffer);
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

	if (info->clustersize == 0) {
		return 0;
	}

	add_random_entropy(par->serialnumber);

	read_fs_info(info);
	kfree_null(&buffer);
	return 1;
}

fat32_t* fat32_mount_volume(const char* device_name)
{
	if (!device_name) {
		return NULL;
	}
	char found_guid[64];
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd) {
		return NULL;
	}
	fat32_t* info = kmalloc(sizeof(fat32_t));
	if (!info) {
		return NULL;
	}
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
		if (success) {
			kprintf("Found FAT32 volume, device %s\n", device_name);
		}
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
		kfree_null(&info);
		return NULL;
	}
	return info;
}

int fat32_attach(const char* device_name, const char* path)
{
	fat32_t* fat32fs = fat32_mount_volume(device_name);
	if (fat32fs) {
		int attached = attach_filesystem(path, fat32_fs, fat32fs);
		if (attached) {
			dprintf("fat32: free space on '%s': %lu bytes\n", path, fs_get_free_space(path));
		}
		return attached;
	}
	return 0;
}

void init_fat32()
{
	fat32_fs = kmalloc(sizeof(filesystem_t));
	if (!fat32_fs) {
		return;
	}
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
	fat32_fs->freespace = fat32_get_free_space;
	register_filesystem(fat32_fs);



}

