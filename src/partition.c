#include <kernel.h>

bool find_partition_of_type(const char* device_name, uint8_t partition_type, uint8_t* partition_id, uint32_t* start, uint32_t* length)
{
	if (partition_id == NULL || start == NULL || length == NULL) {
		return false;
	}
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd && sd->block_size < sizeof(partition_table_t)) {
		dprintf("Couldn't find '%s' or its block size is less than the size of a partition table\n", device_name);
		return false;
	}
	unsigned char* buffer = kmalloc(sd->block_size);
	if (!read_storage_device(device_name, 0, sd->block_size, buffer)) {
		dprintf("Couldn't read boot sector of device '%s'\n", device_name);
		kfree(buffer);
		return false;
	}

	partition_table_t* ptab = (partition_table_t*)(buffer + PARTITION_TABLE_OFFSET);
	for (int i = 0; i < 4; i++) {
		partition_t* p = &(ptab->p_entry[i]);
		if (p->systemid == partition_type) {
			*partition_id = i;
			*start = p->startlba;
			*length = p->length;
			kfree(buffer);
			return true;
		}
	}
	kfree(buffer);
	return false;
}
