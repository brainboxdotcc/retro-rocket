#include "kernel.h"

bool set_fat_entry(fat32_t* info, uint32_t cluster, uint32_t value)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return false;
	}
	uint32_t* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return false;
	}
	uint32_t fat_entry_sector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t fat_entry_offset = (uint32_t) (cluster % (sd->block_size / 4));

	if (!read_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		dprintf("Read failure in set_fat_entry cluster=%08x\n", cluster);
		kfree_null(&buffer);
		return false;
	}
	buffer[fat_entry_offset] = value & 0x0FFFFFFF;
	if (!write_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		dprintf("Write failure in set_fat_entry cluster=%08x to %08x\n", cluster, value);
		kfree_null(&buffer);
		return false;
	}

	kfree_null(&buffer);
	return true;
}

void amend_free_count(fat32_t* info, int adjustment)
{
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return;
	}
	info->info->freecount += adjustment;
	if (info->info->freecount > info->length / info->clustersize) {
		info->info->freecount = info->length / info->clustersize;
	}
	write_storage_device(info->device_name, info->start + info->fsinfocluster, sd->block_size, (unsigned char*)info->info);
}


uint32_t find_next_free_fat_entry(fat32_t* info)
{
	if (!info) {
		return CLUSTER_END;
	}
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return CLUSTER_END;
	}
	uint32_t* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return CLUSTER_END;
	}
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t fat_entry_sector = info->start + info->reservedsectors;

	while (offset < info->fatsize) {
		if (!read_storage_device(info->device_name, fat_entry_sector + offset, sd->block_size, (uint8_t*)buffer)) {
			dprintf("Failed to read sector %x\n", fat_entry_sector + offset);
			kfree_null(&buffer);
			return CLUSTER_END;
		}
		for (size_t t = 0; t < sd->block_size / sizeof(uint32_t); ++t) {
			if (buffer[t] == CLUSTER_FREE) {
				kfree_null(&buffer);
				return count & 0x0FFFFFFF;
			}
			++count;
		}
		++offset;
	}
	kfree_null(&buffer);
	dprintf("No free clusters :(\n");
	return CLUSTER_END;
}

uint32_t get_fat_entry(fat32_t* info, uint32_t cluster)
{
	if (!info) {
		return 0xffffffff;
	}
	storage_device_t* sd = find_storage_device(info->device_name);
	if (!sd) {
		return 0x0fffffff;
	}

	uint32_t* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return 0x0fffffff;
	}
	uint32_t fat_entry_sector = info->start + info->reservedsectors + ((cluster * 4) / sd->block_size);
	uint32_t fat_entry_offset = (uint32_t) (cluster % (sd->block_size / 4));

	if (!read_storage_device(info->device_name, fat_entry_sector, sd->block_size, (uint8_t*)buffer)) {
		kprintf("Read failure in get_fat_entry cluster=%08x\n", cluster);
		kfree_null(&buffer);
		return 0x0fffffff;
	}
	uint32_t entry = buffer[fat_entry_offset] & 0x0FFFFFFF;
	kfree_null(&buffer);
	return entry;
}


