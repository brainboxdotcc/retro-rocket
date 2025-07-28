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

bool binary_to_guid(const void* binary, char* guid)
{
	const uint8_t* unique_id = binary;
	snprintf(
		guid, GUID_ASCII_LEN + 1, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		unique_id[0], unique_id[1], unique_id[2], unique_id[3],
		unique_id[4], unique_id[5],
		unique_id[6], unique_id[7],
		unique_id[8], unique_id[9],
		unique_id[10], unique_id[11], unique_id[12], unique_id[13], unique_id[14], unique_id[15]
	);
	return true;
}

bool scan_gpt_entries(storage_device_t* sd, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length, char* found_guid)
{
	dprintf("*** scanning gpt entries ***\n");
	uint8_t* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return false;
	}
	uint32_t entry_number = 0;
	uint8_t partition_type[16];
	guid_to_binary(partition_type_guid, partition_type);
	if (!read_storage_device(sd->name, 1, sd->block_size, buffer)) {
		dprintf("GPT: Couldn't read second sector\n");
		kfree_null(&buffer);
		return false;
	}
	gpt_header_t* header = (gpt_header_t*)buffer;
	if (memcmp(header->signature, "EFI PART", 8) != 0) {
		dprintf("GPT: No GPT signature found\n");
		kfree_null(&buffer);
		return false;
	}
	dprintf(
		"GPT: Revision: %d, size: %d, number of entries: %d starting at: %d\n",
		header->gpt_revision, header->header_size,
		header->number_partition_entries,
		header->lba_of_partition_entries
	);
	uint8_t* gptbuf = kmalloc(sd->block_size);
	if (!gptbuf) {
		kfree_null(&buffer);
		return false;
	}
	do {
		if (!read_storage_device(sd->name, header->lba_of_partition_entries + entry_number, sd->block_size, gptbuf)) {
			*found_guid = 0;
			kfree_null(&gptbuf);
			kfree_null(&buffer);
			return false;
		}
		gpt_entry_t* gpt = (gpt_entry_t*)gptbuf;
		if (!memcmp(gpt->type_guid, partition_type, 16)) {
			/* Found matching partition */
			dprintf("Found GPT entry at %d, start: %d end: %d\n", entry_number, gpt->start_lba, gpt->end_lba);
			*start = gpt->start_lba;
			*length = gpt->end_lba - gpt->start_lba;
			*partition_id = 0xFF;
			binary_to_guid(gpt->unique_id, found_guid);
			kfree_null(&gptbuf);
			kfree_null(&buffer);
			return true;
		}
		entry_number++;
	} while (entry_number < header->number_partition_entries);
	*found_guid = 0;
	kfree_null(&gptbuf);
	kfree_null(&buffer);
	return false;
}

bool find_partition_of_type(const char* device_name, uint8_t partition_type, char* found_guid, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length)
{
	if (partition_id == NULL || start == NULL || length == NULL) {
		return false;
	}
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd || sd->block_size < sizeof(partition_table_t)) {
		dprintf("Couldn't find '%s' or its block size is less than the size of a partition table\n", device_name);
		return false;
	}
	unsigned char* buffer = kmalloc(sd->block_size);
	if (!buffer) {
		return false;
	}
	if (!read_storage_device(device_name, 0, sd->block_size, buffer)) {
		dprintf("Couldn't read boot sector of device '%s'\n", device_name);
		kfree_null(&buffer);
		return false;
	}

	partition_table_t* ptab = (partition_table_t*)(buffer + PARTITION_TABLE_OFFSET);

	if (ptab->p_entry[0].bootable == 0 && ptab->p_entry[0].systemid == PARTITON_GPT_PROTECTIVE && ptab->p_entry[0].startlba == 1) {
		kfree_null(&buffer);
		return scan_gpt_entries(sd, partition_type_guid, partition_id, start, length, found_guid);
	}

	for (int i = 0; i < 4; i++) {
		partition_t* p = &(ptab->p_entry[i]);
		if (p->systemid == partition_type) {
			*partition_id = i;
			*start = p->startlba;
			*length = p->length;
			*found_guid = 0;
			kfree_null(&buffer);
			return true;
		}
	}
	kfree_null(&buffer);
	return false;
}
