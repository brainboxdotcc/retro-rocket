#include <kernel.h>
#include <guid.h>

/**
 * @brief Read an arbitrary byte range from a block device.
 *
 * Converts a byte offset and length into one or more block reads, copying only
 * the requested byte range into the caller's buffer.
 *
 * This is used by the LVM2 parser because its metadata structures are addressed
 * in bytes rather than whole sectors.
 *
 * @param sd Storage device to read from.
 * @param byte_offset Starting byte offset on the device.
 * @param bytes Number of bytes to read.
 * @param out Destination buffer for the copied data.
 * @return true if all requested bytes were read successfully, otherwise false.
 */
static bool lvm_read_bytes(storage_device_t* sd, uint64_t byte_offset, uint64_t bytes, void* out) {
	uint8_t* dest = out;
	uint8_t block[sd->block_size];

	while (bytes) {
		uint64_t lba = byte_offset / sd->block_size;
		uint64_t offset = byte_offset % sd->block_size;
		uint64_t amount = sd->block_size - offset;
		if (amount > bytes) {
			amount = bytes;
		}
		if (!read_storage_device(sd->name, lba, sd->block_size, block)) {
			return false;
		}
		memcpy(dest, block + offset, amount);
		dest += amount;
		byte_offset += amount;
		bytes -= amount;
	}
	return true;
}

/**
 * @brief Find the value portion of an LVM2 metadata key/value pair.
 *
 * Searches the raw text metadata for a key of the form:
 *
 * key = value
 *
 * and returns a pointer to the first non-whitespace character after the '='.
 *
 * @param start Pointer somewhere inside the raw text metadata blob.
 * @param key Metadata key name to search for.
 * @return Pointer to the start of the value text, or NULL if not found.
 */
static const char* lvm_find_value(const char* start, const char* key) {
	const char* value = strstr(start, key);
	if (!value) {
		return NULL;
	}
	value = strchr(value, '=');
	if (!value) {
		return NULL;
	}
	value++;
	while (*value && isspace(*value)) {
		value++;
	}
	return value;
}

/**
 * @brief Read an unsigned integer value from an LVM2 metadata key/value pair.
 *
 * Searches the raw text metadata for a key of the form:
 *
 * key = value
 *
 * and converts the value portion to a uint64_t.
 *
 * @param start Pointer somewhere inside the raw text metadata blob.
 * @param key Metadata key name to search for.
 * @param value Filled with the parsed integer value.
 * @return true if the key was found and parsed successfully, otherwise false.
 */
static bool lvm_read_uint64(const char* start, const char* key, uint64_t* value) {
	const char* text = lvm_find_value(start, key);
	if (!text) {
		return false;
	}

	*value = strtoul(text, NULL, 10);
	return true;
}

/**
 * @brief Extract the starting physical extent number from an LVM2 stripe definition.
 *
 * Parses the "stripes = [...]" section of a simple linear LVM2 logical volume
 * and returns the starting physical extent index within the physical volume.
 *
 * @param start Pointer somewhere inside the raw text metadata blob.
 * @param pv_extent Filled with the parsed physical extent number.
 * @return true if a usable extent value was found, otherwise false.
 */
static bool lvm_read_pv_extent(const char* start, uint64_t* pv_extent) {
	const char* stripe = strstr(start, "stripes");
	if (!stripe) {
		return false;
	}
	stripe = strchr(stripe, ',');
	if (!stripe) {
		return false;
	}
	stripe++;
	while (*stripe && (isspace(*stripe) || *stripe == '[' || *stripe == '"')) {
		stripe++;
	}
	while (*stripe && *stripe != ',') {
		stripe++;
	}
	if (*stripe != ',') {
		return false;
	}
	stripe++;
	while (*stripe && isspace(*stripe)) {
		stripe++;
	}
	*pv_extent = strtoul(stripe, NULL, 10);
	return true;
}

/**
 * @brief Scan an LVM2 physical volume for supported logical volumes.
 *
 * Parses the raw text LVM2 metadata stored inside a physical volume and walks
 * the logical volume definitions looking for simple linear layouts which can be
 * mapped directly to disk LBAs.
 *
 * Matching logical volumes are exposed as additional visible partition indices
 * alongside ordinary MBR and GPT partitions.
 *
 * @param sd Storage device containing the physical volume.
 * @param pv_start Starting LBA of the LVM2 physical volume.
 * @param pv_length Length of the physical volume in sectors.
 * @param partition_type Requested MBR partition type being searched for.
 * @param partition_type_guid Requested GPT partition type GUID being searched for.
 * @param visible_index Current flattened visible partition index.
 * @param start_index First visible partition index to consider.
 * @param end_index Last visible partition index to consider.
 * @param start Filled with the starting LBA of a matching logical volume.
 * @param length Filled with the length in sectors of a matching logical volume.
 * @param walk If non-NULL, will be called for each item the function iterates over including any match
 * @return true if a matching logical volume was found, otherwise false.
 */
static bool scan_lvm_pv(storage_device_t* sd, uint64_t pv_start, uint64_t pv_length, uint8_t partition_type, const text_guid_t partition_type_guid, uint8_t* visible_index, uint8_t start_index, uint8_t end_index, uint64_t* start, uint64_t* length, volume_enumerator_t* walk) {
	uint8_t sector[sd->block_size];

	if (!read_storage_device(sd->name, pv_start + LVM_LABEL_SECTOR, sd->block_size, sector)) {
		return false;
	}
	lvm_label_header_t* label = (lvm_label_header_t*)sector;
	if (memcmp(label->id, "LABELONE", 8) || memcmp(label->type, "LVM2 001", 8)) {
		return false;
	}

	lvm_pv_header_t* header = (lvm_pv_header_t*)(sector + label->offset);
	uint64_t metadata_offset = header->metadata_area.offset;
	uint64_t metadata_size = header->metadata_area.size;

	/* Prevent absurd metadata sizes from blowing the stack buffer */
	if (!metadata_size || metadata_size > LVM_MAX_METADATA_SIZE) {
		dprintf("LVM: metadata size of %lu is too large or zero\n", metadata_size);
		return false;
	}
	char metadata[metadata_size + 1];
	if (!lvm_read_bytes(sd, (pv_start * sd->block_size) + metadata_offset, metadata_size, metadata)) {
		dprintf("LVM: Failed to read metadata area\n");
		return false;
	}
	metadata[metadata_size] = 0;

	uint64_t extent_size;
	if (!lvm_read_uint64(metadata, "extent_size", &extent_size)) {
		dprintf("LVM: failed to read extent size\n");
		return false;
	}

	const char* p = metadata;

	/*
	 * "p" points somewhere inside the raw text LVM2 metadata blob. We are
	 * walking between logical volume definitions looking for the simplest
	 * possible layout we know how to map directly to LBAs:
	 * one segment, one stripe and starting at extent 0.
	 * Anything more complicated than this is skipped as unsupported
	 */
	while ((p = strstr(p, "segment_count")) != NULL) {
		uint64_t segment_count, stripe_count, start_extent, extent_count, pv_extent;
		if (!lvm_read_uint64(p, "segment_count", &segment_count) || segment_count != 1) {
			p++;
			continue;
		}
		if (!lvm_read_uint64(p, "stripe_count", &stripe_count) || stripe_count != 1) {
			p++;
			continue;
		}
		if (!lvm_read_uint64(p, "start_extent", &start_extent) || start_extent != 0) {
			p++;
			continue;
		}
		if (!lvm_read_uint64(p, "extent_count", &extent_count)) {
			p++;
			continue;
		}
		if (!lvm_read_pv_extent(p, &pv_extent)) {
			p++;
			continue;
		}
		uint64_t lv_start = pv_start + (pv_extent * extent_size);
		uint64_t lv_length = extent_count * extent_size;
		if (lv_start + lv_length > pv_start + pv_length) {
			p++;
			continue;
		}
		/*
		 * LVM2 volumes are Linux-specific, so only ext2/3 style Linux filesystem
		 * searches are allowed to match against logical volumes
		 */
		bool lvm_matches = partition_type == PARTITION_LINUX_FILESYSTEM || match_guid_text(partition_type_guid, GPT_LINUX_FILESYSTEM);

		if (walk && walk->fn) {
			char description[256];
			snprintf(description, sizeof(description), "LVM2 Volume #%02u (%lu MB) Type: LVM2", *visible_index, (lv_length * sd->block_size) / 1024 / 1024);
			walk->fn(visible_index, lvm_matches, description, walk->opaque);
		}

		if (*visible_index >= start_index && *visible_index <= end_index && lvm_matches) {
			*start = lv_start;
			*length = lv_length;
			return true;
		}
		(*visible_index)++;
		p++;
	}
	return false;
}

/**
 * @brief Scan GPT partition entries for matching partitions or supported LVM2 volumes.
 *
 * Walks the GPT partition entry array looking for entries matching the requested
 * partition type GUID. Linux LVM2 physical volumes are also scanned internally
 * so supported logical volumes participate in the same visible partition index
 * numbering as ordinary GPT partitions.
 *
 * @param sd Storage device containing the GPT.
 * @param partition_type_guid Requested GPT partition type GUID to search for.
 * @param partition_type Requested MBR partition type associated with the search.
 * @param partition_id Filled with 0xFF for GPT matches or 0xFE for LVM2 logical volumes.
 * @param start Filled with the starting LBA of a matching partition or logical volume.
 * @param length Filled with the length in sectors of a matching partition or logical volume.
 * @param found_guid Filled with the unique GUID of a matching GPT partition.
 * @param start_index First visible partition index to consider.
 * @param end_index Last visible partition index to consider.
 * @param walk If non-NULL, will be called for each item the function iterates over including any match
 * @return true if a matching partition or logical volume was found, otherwise false.
 */
static bool scan_gpt_entries(storage_device_t* sd, const text_guid_t partition_type_guid, uint8_t partition_type, uint8_t* partition_id, uint64_t* start, uint64_t* length, text_guid_t found_guid, uint8_t start_index, uint8_t end_index, volume_enumerator_t* walk) {
	uint8_t buffer[sd->block_size];

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

	binary_guid_t partition_type_binary;
	binary_guid_t lvm_type;
	guid_to_binary(partition_type_guid, partition_type_binary);
	guid_to_binary(GPT_LINUX_LVM, lvm_type);

	uint32_t entries_per_sector = sd->block_size / header->size_of_each_entry;
	uint8_t gptbuf[sd->block_size];

	uint8_t visible_index = 0;

	uint64_t current_sector = ~0ULL;

	for (uint32_t entry_number = 0; entry_number < header->number_partition_entries; entry_number++) {
		uint64_t sector = header->lba_of_partition_entries + (entry_number / entries_per_sector);
		uint32_t offset  = (entry_number % entries_per_sector) * header->size_of_each_entry;

		/*
		 * GPT entry arrays are usually larger than one sector. Cache the current
		 * sector so the loop can walk individual entries without re-reading the
		 * same block for every partition slot
		 */
		if (sector != current_sector) {
			if (!read_storage_device(sd->name, sector, sd->block_size, gptbuf)) {
				*found_guid = 0;
				return false;
			}
			current_sector = sector;
		}

		gpt_entry_t* gpt = (gpt_entry_t*)(gptbuf + offset);
		size_t partition_length = (gpt->end_lba - gpt->start_lba) + 1;
		bool matched = match_guid_binary(gpt->type_guid, partition_type_binary);
		text_guid_t guid_text;
		binary_to_guid(gpt->type_guid, guid_text);

		if (walk && walk->fn && !match_guid_text(guid_text, "00000000-0000-0000-0000-000000000000")) {
			char description[256];
			snprintf(description, sizeof(description), "GPT Partition #%02u (%lu MB) Type: %s", visible_index, (partition_length * sd->block_size) / 1024 / 1024, guid_text);
			walk->fn(visible_index, matched, description, walk->opaque);
		}

		/*
		 * If we match the GUID we are looking for, we can bail here with a match
		 */
		if (matched && !walk && visible_index >= start_index && visible_index <= end_index) {
			dprintf("Found GPT entry %d, start: %ld end: %ld\n", entry_number, gpt->start_lba, gpt->end_lba);
			*start = gpt->start_lba;
			*length = partition_length;
			*partition_id = 0xFF;
			binary_to_guid(gpt->unique_id, found_guid);
			return true;
		}

		/*
		 * If we find any LVM2 volumes, we must scan inside them treating them as more potential "partitions" by index
		 */
		if (match_guid_binary(gpt->type_guid, lvm_type)) {
			if (scan_lvm_pv(sd, gpt->start_lba, partition_length, partition_type, partition_type_guid, &visible_index, start_index, end_index, start, length, walk) && !walk) {
				*partition_id = 0xFF;
				memcpy(found_guid, partition_type_guid, sizeof(text_guid_t));
				return true;
			}
		}

		visible_index++;
	}

	*found_guid = 0;
	return false;
}

bool find_partition_of_type(const char* device_name, uint8_t partition_type, text_guid_t found_guid, const text_guid_t partition_type_guid, uint8_t* partition_id, uint64_t* start, uint64_t* length, uint8_t start_index, uint8_t end_index, volume_enumerator_t* walk) {
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

	partition_table_t* partition_table = (partition_table_t*)(buffer + PARTITION_TABLE_OFFSET);

	if (partition_table->p_entry[0].bootable == 0 && partition_table->p_entry[0].systemid == PARTITION_GPT_PROTECTIVE && partition_table->p_entry[0].startlba == 1) {
		/*
		 * Once a protective MBR is found, the real partition information lives
		 * in the GPT. Do not continue through the legacy MBR path or the fake
		 * protective entry would pollute the visible partition numbering
		 */
		return scan_gpt_entries(sd, partition_type_guid, partition_type, partition_id, start, length, found_guid, start_index, end_index, walk);
	}

	uint8_t visible_index = 0;
	for (int i = 0; i < 4; i++) {
		partition_t* partition = &(partition_table->p_entry[i]);
		if (walk && walk->fn && partition->systemid != 0 && partition->length > 0) {
			char description[256];
			size_t size = ((size_t)partition->length * (size_t)sd->block_size) / 1024 / 1024;
			snprintf(description, sizeof(description), "BIOS Partition #%02u (%lu MB) Type: &%02x", visible_index, size, partition->systemid);
			walk->fn(visible_index, partition->systemid == partition_type, description, walk->opaque);
		}
		/*
		 * If we match against a traditional BIOS partition ID, then we can bail here with a positive match
		 */
		if (partition->systemid == partition_type && !walk && visible_index >= start_index && visible_index <= end_index) {
			*partition_id = i;
			*start = partition->startlba;
			*length = partition->length;
			*found_guid = 0;
			return true;
		}
		/*
		 * If we find any LVM2 volumes, we must scan inside them treating them as more potential "partitions" by index
		 */
		if (partition->systemid == PARTITION_LVM) {
			if (scan_lvm_pv(sd, partition->startlba, partition->length, partition_type, partition_type_guid, &visible_index, start_index, end_index, start, length, walk) && !walk) {
				*partition_id = partition_type;
				*found_guid = 0;
				return true;
			}
		}

		visible_index++;

	}
	return false;
}