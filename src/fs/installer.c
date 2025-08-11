#include <kernel.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "partition.h"  /* gpt_header_t, gpt_entry_t, partition_table_t, GUID helpers */

/* GPT type GUID strings already provided by kernel.h:
 *   GPT_EFI_SYSTEM
 *   RFS_GPT_GUID
 */

#define GPT_SIGNATURE_TEXT         "EFI PART"
#define GPT_REVISION_1_0           0x00010000U
#define GPT_HEADER_SIZE_BYTES      92U
#define GPT_PTE_COUNT              128U
#define GPT_PTE_SIZE_BYTES         128U

#define ALIGN_1M_IN_LBAS           2048ULL     /* 1 MiB @ 512 B/LBA */
#define SECTOR_BYTES_REQUIRED      512U
#define MAX_WRITE_CHUNK_BYTES      (128U * 1024U)
#define FAT32_BPB_HIDDEN_OFF       0x1C        /* LE u32 */

/* ---------------- CRC32 (poly 0xEDB88320) ---------------- */

static uint32_t crc32_update(uint32_t crc, const void *data, size_t len)
{
	static uint32_t table[256];
	static bool init = false;

	if (!init) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int j = 0; j < 8; j++) {
				if ((c & 1U) != 0U) {
					c = 0xEDB88320U ^ (c >> 1);
				} else {
					c >>= 1;
				}
			}
			table[i] = c;
		}
		init = true;
	}

	uint32_t c = crc ^ 0xFFFFFFFFU;
	const uint8_t *p = (const uint8_t *)data;

	for (size_t i = 0; i < len; i++) {
		c = table[(c ^ p[i]) & 0xFFU] ^ (c >> 8);
	}

	return c ^ 0xFFFFFFFFU;
}

/* ---------------- small helpers ---------------- */

static uint64_t align_up_u64(uint64_t x, uint64_t a)
{
	if (a == 0ULL) {
		return x;
	}
	return (x + (a - 1ULL)) & ~(a - 1ULL);
}

static void make_utf16le_name(const char *ascii, uint16_t out36[36])
{
	size_t n = strlen(ascii);
	if (n > 36U) {
		n = 36U;
	}
	for (size_t i = 0; i < n; i++) {
		out36[i] = (uint16_t)(uint8_t)ascii[i];
	}
	for (size_t i = n; i < 36U; i++) {
		out36[i] = 0;
	}
}

/* very simple PRNG; replace with your kernel RNG if available */
static uint32_t prng_state = 0xC0FFEE01U;

static uint32_t prng32(void)
{
	prng_state = prng_state * 1664525U + 1013904223U;
	return prng_state;
}

static void random_guid_v4(uint8_t out16[16])
{
	for (int i = 0; i < 16; i += 4) {
		uint32_t r = prng32();
		memcpy(out16 + i, &r, 4);
	}
	out16[6] = (out16[6] & 0x0F) | 0x40; /* version 4 */
	out16[8] = (out16[8] & 0x3F) | 0x80; /* variant 10 */
}

static bool write_lbas(const char *devname, uint64_t start_lba, const void *buf, uint32_t bytes, uint32_t sector_bytes)
{
	if ((bytes % sector_bytes) != 0U) {
		return false;
	}
	if (write_storage_device(devname, start_lba, bytes, (const unsigned char *)buf) == 0) {
		return false;
	}
	return true;
}

/**
 * @brief Flatten a device, write GPT with ESP + RFS, then write a prebuilt ESP image.
 *
 * @param devname             storage device name (e.g. "hd0")
 * @param esp_image_vfs_path  VFS path to prebuilt FAT32 ESP image (~68 MiB)
 * @return true on success
 */
bool install_gpt_esp_rfs_whole_image(const char *devname, const char *esp_image_vfs_path)
{
	storage_device_t *dev = find_storage_device(devname);
	if (dev == NULL) {
		kprintf("install: device '%s' not found\n", devname);
		return false;
	}

	if (dev->block_size != SECTOR_BYTES_REQUIRED) {
		kprintf("install: unsupported logical sector size %u (need 512)\n", dev->block_size);
		return false;
	}
	if (dev->size == SIZE_MAX || dev->size < 34) {
		kprintf("install: unknown or too-small device size\n");
		return false;
	}

	const uint32_t sector_bytes = dev->block_size;
	const uint64_t lba_count    = dev->size;
	const uint64_t last_lba     = lba_count - 1;

	/* --- Get ESP image info --- */
	fs_directory_entry_t *img_ent = fs_get_file_info(esp_image_vfs_path);
	if (img_ent == NULL) {
		kprintf("install: ESP image '%s' not found\n", esp_image_vfs_path);
		return false;
	}

	uint64_t esp_bytes = img_ent->size;
	if (esp_bytes == 0ULL) {
		kprintf("install: ESP image is empty\n");
		return false;
	}
	if (esp_bytes > 0xFFFFFFFFULL) {
		kprintf("install: ESP image too large for single-file logic\n");
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
		kprintf("install: disk too small for ESP (%lu sectors ESP, esp_last_lba=%lu last_usable=%lu)\n", esp_sectors, esp_last_lba, last_usable);
		return false;
	}

	uint64_t rfs_first_lba = align_up_u64(esp_last_lba + 1ULL, ALIGN_1M_IN_LBAS);
	if (rfs_first_lba > last_usable) {
		kprintf("install: no room for RFS after ESP\n");
		return false;
	}
	const uint64_t rfs_last_lba = last_usable;

	/* --- Protective MBR (LBA0) --- */
	uint8_t mbr[SECTOR_BYTES_REQUIRED];
	memset(mbr, 0, sizeof mbr);
	partition_table_t *ptab = (partition_table_t *)(mbr + PARTITION_TABLE_OFFSET);
	memset(ptab, 0, sizeof(*ptab));

	ptab->p_entry[0].bootable = 0x00;
	ptab->p_entry[0].systemid = PARTITION_GPT_PROTECTIVE;
	ptab->p_entry[0].startlba = 1;
	ptab->p_entry[0].length   = (lba_count - 1 > 0xFFFFFFFF) ? 0xFFFFFFFF : (uint32_t)(lba_count - 1);

	mbr[510] = 0x55;
	mbr[511] = 0xAA;

	if (!write_lbas(devname, 0ULL, mbr, sizeof mbr, sector_bytes)) {
		kprintf("install: write protective MBR failed\n");
		return false;
	}

	/* --- Partition Entries --- */
	const uint32_t ptes_buf_bytes = (uint32_t)(ptes_sectors * sector_bytes);
	uint8_t *ptes_buf = (uint8_t *)kmalloc(ptes_buf_bytes);
	if (!ptes_buf) {
		kprintf("install: out of memory for PTE buffer\n");
		return false;
	}
	memset(ptes_buf, 0, ptes_buf_bytes);
	gpt_entry_t *ptes = (gpt_entry_t *)ptes_buf;

	/* Entry 0: EFI System */
	if (!guid_to_binary(GPT_EFI_SYSTEM, ptes[0].type_guid)) {
		kprintf("install: guid_to_binary failed for GPT_EFI_SYSTEM\n");
		kfree(ptes_buf);
		return false;
	}
	random_guid_v4(ptes[0].unique_id);
	ptes[0].start_lba  = esp_first_lba;
	ptes[0].end_lba    = esp_last_lba;
	make_utf16le_name("EFI System", (uint16_t *)ptes[0].name);

	/* Entry 1: RetroFS */
	if (!guid_to_binary(RFS_GPT_GUID, ptes[1].type_guid)) {
		kprintf("install: guid_to_binary failed for RFS_GPT_GUID\n");
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
	memset(gpth_buf, 0, sizeof gpth_buf);
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
	memset(gptb_buf, 0, sizeof gptb_buf);
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
		kprintf("install: failed writing GPT structures\n");
		kfree(ptes_buf);
		return false;
	}
	kfree(ptes_buf);

	/* --- Stream ESP image directly --- */
	const uint32_t chunk_bytes = 64 * 1024;
	uint8_t *buf16k = (uint8_t *)kmalloc(chunk_bytes);
	if (!buf16k) {
		kprintf("install: out of memory for ESP streaming buffer\n");
		return false;
	}

	uint64_t remaining_bytes = esp_bytes;
	uint64_t read_offset     = 0ULL;
	uint64_t out_lba         = esp_first_lba;
	kprintf("FAT image total sectors to write: %lu\n", esp_sectors);

	while (remaining_bytes > 0) {
		memset(buf16k, (unsigned char)0xCE, chunk_bytes); // test sentinel
		if (!fs_read_file(img_ent, (uint32_t)read_offset, chunk_bytes, buf16k)) {
			kprintf("install: fs_read_file failed at offset %lu: %s\n", read_offset, fs_strerror(fs_get_error()));
			kfree(buf16k);
			return false;
		}

		uint32_t n_sectors = chunk_bytes / sector_bytes;
		if (!write_lbas(devname, out_lba, buf16k, chunk_bytes, sector_bytes)) {
			kprintf("install: write failed at LBA %lu\n", out_lba);
			kfree(buf16k);
			return false;
		}

		read_offset += chunk_bytes;
		out_lba     += n_sectors;
		remaining_bytes = (remaining_bytes > chunk_bytes) ? (remaining_bytes - chunk_bytes) : 0;
	}

	kfree(buf16k);

	kprintf("install: GPT + ESP + RFS written to '%s'\n", devname);
	kprintf("         ESP  %lu..%lu (%lu sectors)\n", esp_first_lba, esp_last_lba, esp_sectors);
	kprintf("         RFS  %lu..%lu\n", rfs_first_lba, rfs_last_lba);

	/* Now format the partition */

	bool success = false;
	uint8_t partitionid = 0;
	char found_guid[64];
	rfs_t *info = kmalloc(sizeof(rfs_t));
	memset(info, 0, sizeof(rfs_t));
	uint64_t start = 0, length = 0;
	info->dev = dev;

	if (!find_partition_of_type(devname, 0xFF, found_guid, RFS_GPT_GUID, &partitionid, &start, &length)) {
		kprintf("install: Could not find the created RFS to format it on %s\n", devname);
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
		kprintf("install: RFS on device %s formatted successfully\n", devname);
	} else {
		kprintf("install: Failed to format RFS on device %s\n", devname);
		wait_forever();
	}
	kfree_null(&info);

	/* Mount the newly formatted volume */
	filesystem_mount("/harddisk", devname, "rfs");

	/* TODO: Copy OS files onto RFS; /system, an empty /devices, /programs */
	kprintf("Copying files...\n");

	return true;
}

void installer() {
	kprintf("Installer stub\n");
	install_gpt_esp_rfs_whole_image("hd0", "/efi.fat");
	wait_forever();
}