#include <kernel.h>

bool guid_to_binary(const char* guid, void* binary)
{
	uint16_t* words = binary;
	char str_word[5] = { 0 };
	uint64_t count = 0;
	// Process 4 hex digits at a time, to the maximum of 32
	while (*guid && count < 32) {
		while (*guid == '-') guid++;
		memcpy(str_word, guid, 4);
		*(words++) = htons((uint16_t)(atoll(str_word, 16) & 0xFFFF));
		guid += 4;
		count += 4;
	}
	return true;
}

bool scan_gpt_entries(storage_device_t* sd, const char* partition_type_guid, uint8_t* partition_id, uint32_t* start, uint32_t* length)
{
	unsigned char* buffer = kmalloc(sd->block_size);
	kfree(buffer);
	return false;
}

bool find_partition_of_type(const char* device_name, uint8_t partition_type, const char* partition_type_guid, uint8_t* partition_id, uint32_t* start, uint32_t* length)
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

	if (ptab->p_entry[0].bootable == 0 && ptab->p_entry[0].systemid == PARTITON_GPT_PROTECTIVE && ptab->p_entry[0].startlba == 1) {
		kfree(buffer);
		return scan_gpt_entries(sd, partition_type_guid, partition_id, start, length);
	}

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
