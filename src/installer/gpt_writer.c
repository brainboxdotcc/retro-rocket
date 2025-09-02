/**
 * @file gpt_writer.c
 * @author Craig Edwards
 * @brief GPT, ESP and layout writer for installer
 * @copyright Copyright (c) 2012-2025
 */
#include <installer.h>

bool install_gpt_esp_rfs_whole_image(const char *devname, const char *esp_image_vfs_path) {
	storage_device_t *dev = find_storage_device(devname);
	if (dev == NULL) {
		error_page("device '%s' not found\n", devname);
		return false;
	}

	if (dev->block_size != SECTOR_BYTES_REQUIRED) {
		error_page("unsupported logical sector size %u (need 512)\n", dev->block_size);
		return false;
	}
	if (dev->size == SIZE_MAX || dev->size < 34) {
		error_page("unknown or too-small device size\n");
		return false;
	}

	const uint32_t sector_bytes = dev->block_size;
	const uint64_t lba_count    = dev->size;
	const uint64_t last_lba     = lba_count - 1;

	/* --- Get ESP image info --- */
	fs_directory_entry_t *img_ent = fs_get_file_info(esp_image_vfs_path);
	if (img_ent == NULL) {
		error_page("ESP image '%s' not found", esp_image_vfs_path);
		return false;
	}

	uint64_t esp_bytes = img_ent->size;
	if (esp_bytes == 0ULL) {
		error_page("ESP image is empty");
		return false;
	}
	if (esp_bytes > 0xFFFFFFFFULL) {
		error_page("ESP image too large for single-file logic");
		return false;
	}

	const uint64_t esp_sectors = (esp_bytes + (uint64_t)sector_bytes - 1ULL) / (uint64_t)sector_bytes;

	/* --- GPT layout --- */
	const uint32_t ptes_bytes_total = GPT_PTE_COUNT * GPT_PTE_SIZE_BYTES;
	const uint64_t ptes_sectors     = (ptes_bytes_total + sector_bytes - 1U) / sector_bytes;

	const uint64_t primary_header_lba = 1ULL;
	const uint64_t primary_ptes_lba   = 2ULL;
	const uint64_t backup_ptes_lba    = last_lba - ptes_sectors + 1ULL;
	const uint64_t backup_header_lba  = last_lba;

	uint64_t first_usable = primary_ptes_lba + ptes_sectors;
	if (first_usable < 34ULL) {
		first_usable = 34ULL;
	}
	first_usable = align_up_u64(first_usable, ALIGN_1M_IN_LBAS);

	const uint64_t last_usable = backup_ptes_lba - 1ULL;

	const uint64_t esp_first_lba = first_usable;
	const uint64_t esp_last_lba  = esp_first_lba + esp_sectors - 1ULL;

	if (esp_last_lba >= last_usable) {
		error_page("disk too small for ESP (%lu sectors ESP, esp_last_lba=%lu last_usable=%lu)", esp_sectors, esp_last_lba, last_usable);
		return false;
	}

	uint64_t rfs_first_lba = align_up_u64(esp_last_lba + 1ULL, ALIGN_1M_IN_LBAS);
	if (rfs_first_lba > last_usable) {
		error_page("no room for RFS after ESP");
		return false;
	}
	const uint64_t rfs_last_lba = last_usable;

	/* --- Protective MBR (LBA0) --- */
	uint8_t mbr[SECTOR_BYTES_REQUIRED];
	memset(mbr, 0, sizeof(mbr));
	partition_table_t *ptab = (partition_table_t *)(mbr + PARTITION_TABLE_OFFSET);
	memset(ptab, 0, sizeof(*ptab));

	ptab->p_entry[0].bootable = 0x00;
	ptab->p_entry[0].systemid = PARTITION_GPT_PROTECTIVE;
	ptab->p_entry[0].startlba = 1;
	ptab->p_entry[0].length   = (lba_count - 1 > 0xFFFFFFFF) ? 0xFFFFFFFF : (uint32_t)(lba_count - 1);

	mbr[510] = 0x55;
	mbr[511] = 0xAA;

	if (!write_lbas(devname, 0ULL, mbr, sizeof(mbr), sector_bytes)) {
		error_page("write protective MBR failed");
		return false;
	}

	/* --- Partition Entries --- */
	const uint32_t ptes_buf_bytes = (ptes_sectors * sector_bytes);
	uint8_t *ptes_buf = kmalloc(ptes_buf_bytes);
	if (!ptes_buf) {
		error_page("out of memory for PTE buffer");
		return false;
	}
	memset(ptes_buf, 0, ptes_buf_bytes);
	gpt_entry_t *ptes = (gpt_entry_t *)ptes_buf;

	/* Entry 0: EFI System */
	if (!guid_to_binary(GPT_EFI_SYSTEM, ptes[0].type_guid)) {
		error_page("guid_to_binary failed for GPT_EFI_SYSTEM");
		kfree(ptes_buf);
		return false;
	}
	random_guid_v4(ptes[0].unique_id);
	ptes[0].start_lba  = esp_first_lba;
	ptes[0].end_lba    = esp_last_lba;
	make_utf16le_name("EFI System", (uint16_t *)ptes[0].name);

	/* Entry 1: RetroFS */
	if (!guid_to_binary(RFS_GPT_GUID, ptes[1].type_guid)) {
		error_page("guid_to_binary failed for RFS_GPT_GUID");
		kfree(ptes_buf);
		return false;
	}
	random_guid_v4(ptes[1].unique_id);
	ptes[1].start_lba  = rfs_first_lba;
	ptes[1].end_lba    = rfs_last_lba;
	make_utf16le_name("RetroFS", (uint16_t *)ptes[1].name);

	const uint32_t ptes_crc = crc32_update(0, ptes_buf, GPT_PTE_COUNT * GPT_PTE_SIZE_BYTES);

	/* --- GPT Headers --- */
	uint8_t gpth_buf[SECTOR_BYTES_REQUIRED];
	memset(gpth_buf, 0, sizeof(gpth_buf));
	gpt_header_t *gpth = (gpt_header_t *)gpth_buf;
	memcpy(gpth->signature, GPT_SIGNATURE_TEXT, 8);
	gpth->gpt_revision              = GPT_REVISION_1_0;
	gpth->header_size               = GPT_HEADER_SIZE_BYTES;
	gpth->lba_of_this_header        = primary_header_lba;
	gpth->lba_of_alternative_header = backup_header_lba;
	gpth->first_usable_block        = first_usable;
	gpth->last_usable_block         = last_usable;
	random_guid_v4(gpth->disk_guid);
	gpth->lba_of_partition_entries  = primary_ptes_lba;
	gpth->number_partition_entries  = GPT_PTE_COUNT;
	gpth->size_of_each_entry        = GPT_PTE_SIZE_BYTES;
	gpth->crc_32_of_entries         = ptes_crc;
	gpth->crc_checksum              = 0;
	gpth->crc_checksum              = crc32_update(0, gpth, GPT_HEADER_SIZE_BYTES);

	uint8_t gptb_buf[SECTOR_BYTES_REQUIRED];
	memset(gptb_buf, 0, sizeof(gptb_buf));
	gpt_header_t *gptb = (gpt_header_t *)gptb_buf;
	memcpy(gptb, gpth, GPT_HEADER_SIZE_BYTES);
	gptb->lba_of_this_header        = backup_header_lba;
	gptb->lba_of_alternative_header = primary_header_lba;
	gptb->lba_of_partition_entries  = backup_ptes_lba;
	gptb->crc_checksum              = 0;
	gptb->crc_checksum              = crc32_update(0, gptb, GPT_HEADER_SIZE_BYTES);

	/* --- Write GPT structures --- */
	if (!write_lbas(devname, primary_ptes_lba, ptes_buf, ptes_buf_bytes, sector_bytes) ||
	    !write_lbas(devname, primary_header_lba, gpth_buf, SECTOR_BYTES_REQUIRED, sector_bytes) ||
	    !write_lbas(devname, backup_ptes_lba, ptes_buf, ptes_buf_bytes, sector_bytes) ||
	    !write_lbas(devname, backup_header_lba, gptb_buf, SECTOR_BYTES_REQUIRED, sector_bytes)) {
		error_page("failed writing GPT structures");
		kfree(ptes_buf);
		return false;
	}
	kfree(ptes_buf);

	/* --- Stream ESP image directly --- */
	const uint32_t chunk_bytes = 64 * 1024;
	uint8_t *esp_buffer = kmalloc(chunk_bytes);
	if (!esp_buffer) {
		error_page("out of memory for ESP streaming buffer");
		return false;
	}

	uint64_t remaining_bytes = esp_bytes;
	uint64_t read_offset     = 0ULL;
	uint64_t out_lba         = esp_first_lba;
	uint64_t steps           = 0;
	display_progress("Installing recovery/boot image (step 1 of 3)", 0);

	while (remaining_bytes > 0) {
		memset(esp_buffer, 0, chunk_bytes);
		if (!fs_read_file(img_ent, (uint32_t)read_offset, chunk_bytes, esp_buffer)) {
			error_page("fs_read_file failed at offset %lu: %s", read_offset, fs_strerror(fs_get_error()));
			kfree(esp_buffer);
			return false;
		}

		uint32_t n_sectors = chunk_bytes / sector_bytes;
		if (!write_lbas(devname, out_lba, esp_buffer, chunk_bytes, sector_bytes)) {
			error_page("write failed at LBA %lu", out_lba);
			kfree(esp_buffer);
			return false;
		}

		read_offset += chunk_bytes;
		out_lba     += n_sectors;
		remaining_bytes = (remaining_bytes > chunk_bytes) ? (remaining_bytes - chunk_bytes) : 0;

		if (steps++ % 25 == 0) {
			display_progress("Installing recovery/boot image (step 1 of 3)", ((esp_bytes - remaining_bytes) * 100) / esp_bytes);
		}
	}

	kfree(esp_buffer);

	return prepare_rfs_partition(dev);
}

bool prepare_rfs_partition(storage_device_t* dev) {
	bool success = false;
	uint8_t partitionid = 0;
	char found_guid[64];
	rfs_t *info = kmalloc(sizeof(rfs_t));
	if (!info) {
		return false;
	}
	memset(info, 0, sizeof(rfs_t));
	uint64_t start = 0, length = 0;
	info->dev = dev;

	display_progress("Formatting RetroFS partition (step 2 of 3)", 33);

	/* Find the RFS partition on the device */
	if (!find_partition_of_type(dev->name, 0xFF, found_guid, RFS_GPT_GUID, &partitionid, &start, &length)) {
		error_page("Could not find the created RFS to format it on %s", dev->name);
	} else {
		info->start = start;
		info->length = length;
		info->total_sectors = length;
		/* 0xFF means we got a GPT partition not MBR partition; only GPT partitions are supported for RFS. */
		if (partitionid == 0xFF) {
			success = rfs_format(info);
		}
	}

	if (success) {
		display_progress("RFS on device %s formatted successfully", 44);
	} else {
		error_page("Failed to format RFS on device %s", dev->name);
	}
	kfree_null(&info);
	return success;
}
