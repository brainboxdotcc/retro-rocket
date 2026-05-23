#include <kernel.h>

static inline uint8_t hexval(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
	return 0;
}

static inline uint8_t parse_byte(const char* s) {
	return (hexval(s[0]) << 4) | hexval(s[1]);
}

bool guid_to_binary(const char* guid, void* binary) {
	uint8_t* out = binary;

	// field 1: 8 hex digits, little-endian
	out[3] = parse_byte(guid + 0);
	out[2] = parse_byte(guid + 2);
	out[1] = parse_byte(guid + 4);
	out[0] = parse_byte(guid + 6);
	guid += 8 + 1; // skip "XXXXXXXX-"

	// field 2: 4 hex digits, little-endian
	out[5] = parse_byte(guid + 0);
	out[4] = parse_byte(guid + 2);
	guid += 4 + 1; // skip "XXXX-"

	// field 3: 4 hex digits, little-endian
	out[7] = parse_byte(guid + 0);
	out[6] = parse_byte(guid + 2);
	guid += 4 + 1; // skip "XXXX-"

	// field 4: 4 hex digits, big-endian (as-is)
	out[8] = parse_byte(guid + 0);
	out[9] = parse_byte(guid + 2);
	guid += 4 + 1; // skip "XXXX-"

	// field 5: 12 hex digits, big-endian (as-is)
	for (int i = 0; i < 6; i++) {
		out[10 + i] = parse_byte(guid + i*2);
	}

	return true;
}

bool binary_to_guid(const void* binary, char* guid) {
	const uint8_t* in = binary;
	unsigned int d1 = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
	unsigned int d2 = in[4] | (in[5] << 8);
	unsigned int d3 = in[6] | (in[7] << 8);
	snprintf(guid, GUID_ASCII_LEN + 1, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", d1, d2, d3, in[8], in[9], in[10], in[11], in[12], in[13], in[14], in[15]);
	return true;
}



bool scan_gpt_entries(storage_device_t* sd, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length, char* found_guid, uint8_t start_index, uint8_t end_index) {
	uint8_t buffer[sd->block_size];

	// Read GPT header
	if (!read_storage_device(sd->name, 1, sd->block_size, buffer)) {
		dprintf("GPT: Couldn't read second sector\n");
		return false;
	}

	gpt_header_t* header = (gpt_header_t*)buffer;
	if (memcmp(header->signature, "EFI PART", 8) != 0) {
		dprintf("GPT: No GPT signature found\n");
		return false;
	}

	dprintf("GPT: Revision: %d, size: %d, number of entries: %d starting at LBA: %ld, entry size: %d\n", header->gpt_revision, header->header_size, header->number_partition_entries, header->lba_of_partition_entries, header->size_of_each_entry);

	uint8_t partition_type[GUID_BINARY_LEN];
	guid_to_binary(partition_type_guid, partition_type);

	uint32_t entries_per_sector = sd->block_size / header->size_of_each_entry;
	uint8_t gptbuf[sd->block_size];

	uint64_t current_sector = ~0ULL; // force load first
	for (uint32_t entry_number = 0; entry_number < header->number_partition_entries; entry_number++) {
		uint64_t sector = header->lba_of_partition_entries + (entry_number / entries_per_sector);
		uint32_t offset  = (entry_number % entries_per_sector) * header->size_of_each_entry;

		if (sector != current_sector) {
			if (!read_storage_device(sd->name, sector, sd->block_size, gptbuf)) {
				*found_guid = 0;
				return false;
			}
			current_sector = sector;
		}

		if (entry_number < start_index || entry_number > end_index) {
			continue;
		}

		gpt_entry_t* gpt = (gpt_entry_t*)(gptbuf + offset);

		if (!memcmp(gpt->type_guid, partition_type, GUID_BINARY_LEN)) {
			// Found matching partition
			dprintf("Found GPT entry %d, start: %ld end: %ld\n", entry_number, gpt->start_lba, gpt->end_lba);

			*start = gpt->start_lba;
			*length = (gpt->end_lba - gpt->start_lba) + 1; // inclusive LBA range
			*partition_id = 0xFF;
			binary_to_guid(gpt->unique_id, found_guid);
			return true;
		}
	}

	*found_guid = 0;
	return false;
}

bool find_partition_of_type(const char* device_name, uint8_t partition_type, char* found_guid, const char* partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length, uint8_t start_index, uint8_t end_index) {
	if (partition_id == NULL || start == NULL || length == NULL) {
		return false;
	}
	storage_device_t* sd = find_storage_device(device_name);
	if (!sd || sd->block_size < sizeof(partition_table_t)) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return false;
	}
	unsigned char buffer[sd->block_size];
	if (!read_storage_device(device_name, 0, sd->block_size, buffer)) {
		return false;
	}

	partition_table_t* ptab = (partition_table_t*)(buffer + PARTITION_TABLE_OFFSET);

	if (ptab->p_entry[0].bootable == 0 && ptab->p_entry[0].systemid == PARTITION_GPT_PROTECTIVE && ptab->p_entry[0].startlba == 1) {
		return scan_gpt_entries(sd, partition_type_guid, partition_id, start, length, found_guid, start_index, end_index);
	}

	for (int i = 0; i < 4; i++) {
		if (i < start_index || i > end_index) {
			continue;
		}
		partition_t* p = &(ptab->p_entry[i]);
		if (p->systemid == partition_type) {
			*partition_id = i;
			*start = p->startlba;
			*length = p->length;
			*found_guid = 0;
			return true;
		}
	}
	return false;
}
