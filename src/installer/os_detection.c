/**
 * @file os_detection.c
 * @author Craig Edwards
 * @brief Detects what OS is currently installed on a device
 * @copyright Copyright (c) 2012-2025
 */
#include <installer.h>

static const size_t gpt_guid_map_count = sizeof(gpt_guid_map) / sizeof(gpt_guid_map[0]);

bool probe_device_summary(const storage_device_t *dev, char *out, size_t out_len, bool *usable) {
	const char* devname = dev->name;
	uint32_t sec = dev->block_size;
	uint64_t lba_count = dev->size;
	uint8_t mbr[512];
	bool ok = false;

	if (usable != NULL) {
		*usable = false;
	}

	if (out_len > 0) {
		out[0] = '\0';
	}

	if (sec != 512 || lba_count == SIZE_MAX || lba_count < 2) {
		snprintf(out, out_len, "Could not inspect this drive.");
		return true;
	}

	if (!read_storage_device(devname, 0, sizeof(mbr), mbr)) {
		snprintf(out, out_len, "Could not read this drive.");
		return true;
	}

	if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
		snprintf(out, out_len, "Empty drive");
		if (usable != NULL) {
			*usable = true;
		}
		return true;
	}

	partition_table_t *pt = (partition_table_t *)(mbr + PARTITION_TABLE_OFFSET);
	bool has_protective = false;
	int mbr_parts = 0;
	uint8_t mbr_types[4] = {0};
	uint32_t mbr_len[4] = {0};

	for (int i = 0; i < 4; i++) {
		uint8_t type = pt->p_entry[i].systemid;
		if (type == 0) {
			continue;
		}
		if (type == PARTITION_GPT_PROTECTIVE) {
			has_protective = true;
		}
		mbr_types[mbr_parts] = type;
		mbr_len[mbr_parts] = pt->p_entry[i].length;
		mbr_parts++;
	}

	if (has_protective) {
		uint8_t hdr[512];

		if (!read_storage_device(devname, 1, sizeof(hdr), hdr)) {
			snprintf(out, out_len, "Could not read this drive.");
			return false;
		}

		gpt_header_t *gh = (gpt_header_t *)hdr;

		if (memcmp(gh->signature, GPT_SIGNATURE_TEXT, 8) != 0 || gh->size_of_each_entry != GPT_PTE_SIZE_BYTES) {
			snprintf(out, out_len, "Empty drive");
			return true;
		}

		uint32_t count = gh->number_partition_entries;
		uint64_t ptes_lba = gh->lba_of_partition_entries;
		uint32_t table_bytes = count * gh->size_of_each_entry;

		if (count == 0) {
			snprintf(out, out_len, "Empty drive");
			if (usable != NULL) {
				*usable = true;
			}
			return true;
		}
		if (count > GPT_PTE_COUNT || table_bytes > 1024 * 1024) {
			snprintf(out, out_len, "Partition table looks unusual.");
			return true;
		}

		uint8_t *ptes = kmalloc(table_bytes);
		if (ptes == NULL) {
			snprintf(out, out_len, "Out of memory while inspecting.");
			return true;
		}

		bool rd = read_storage_device(devname, ptes_lba, table_bytes, ptes);
		if (!rd) {
			kfree(ptes);
			snprintf(out, out_len, "Could not read this drive.");
			return true;
		}

		uint8_t guids[gpt_guid_map_count][16];
		for (size_t i = 0; i < gpt_guid_map_count; i++) {
			guid_to_binary(gpt_guid_map[i].guid_text, guids[i]);
		}

		char tags[16][20];
		int tagc = 0;
		uint64_t biggest = 0;
		const char *likely = "Unknown";

		for (uint32_t i = 0; i < count; i++) {
			gpt_entry_t *e = (gpt_entry_t *)(ptes + i * gh->size_of_each_entry);
			if (e->start_lba == 0 && e->end_lba == 0) {
				continue;
			}

			const char *t = "Other";
			const char *os = NULL;

			for (size_t m = 0; m < gpt_guid_map_count; m++) {
				if (memcmp(e->type_guid, guids[m], 16) == 0) {
					t = gpt_guid_map[m].tag;
					os = gpt_guid_map[m].likely_os;
					break;
				}
			}

			if (tagc < 16) {
				snprintf(tags[tagc], sizeof(tags[0]), "%s", t);
				tagc++;
			}

			if (os != NULL) {
				uint64_t len = 0;
				if (e->end_lba >= e->start_lba) {
					len = e->end_lba - e->start_lba + 1;
				}
				if (len > biggest) {
					biggest = len;
					likely = os;
				}
			}
		}

		kfree(ptes);

		if (tagc == 0) {
			snprintf(out, out_len, "Nothing recognisable on this drive.");
		} else {
			char list[128] = {0};
			size_t pos = 0;

			for (int i = 0; i < tagc; i++) {
				int n = snprintf(list + pos, sizeof(list) - pos, "%s%s", tags[i], (i + 1 < tagc ? ", " : ""));
				if (n < 0) {
					break;
				}
				pos += n;
				if (pos >= sizeof(list)) {
					break;
				}
			}

			if (strcmp(likely, "Unknown") == 0) {
				strlcpy(out, list, out_len);
			} else {
				strlcpy(out, likely, out_len);
			}
		}

		ok = true;
	} else {
		if (mbr_parts == 0) {
			snprintf(out, out_len, "Empty drive");
			if (usable != NULL) {
				*usable = true;
			}
			return true;
		}

		char tags[4][20];
		int tagc = 0;
		uint32_t biggest = 0;
		const char *likely = "Unknown";

		for (int i = 0; i < mbr_parts; i++) {
			uint8_t t = mbr_types[i];
			const char *label = "Other";
			const char *os = NULL;

			for (size_t i = 0; i < sizeof(mbr_id_map) / sizeof(mbr_id_map[0]); i++) {
				if (t == mbr_id_map[i].mbr_type) {
					label = mbr_id_map[i].label;
					os    = mbr_id_map[i].os;
					break;
				}
			}

			if (tagc < 4) {
				snprintf(tags[tagc], sizeof(tags[0]), "%s", label);
				tagc++;
			}

			if (os != NULL) {
				if (mbr_len[i] > biggest) {
					biggest = mbr_len[i];
					likely = os;
				}
			}
		}

		char list[64] = {0};
		size_t pos = 0;

		for (int i = 0; i < tagc; i++) {
			int n = snprintf(list + pos, sizeof(list) - pos, "%s%s", tags[i], (i + 1 < tagc ? ", " : ""));
			if (n < 0) {
				break;
			}
			pos += n;
			if (pos >= sizeof(list)) {
				break;
			}
		}

		if (tagc == 0) {
			snprintf(out, out_len, "Unknown operating system");
		} else if (strcmp(likely, "Unknown") == 0) {
			strlcpy(out, list, out_len);
		} else {
			strlcpy(out, likely, out_len);
		}

		ok = true;
	}

	if (usable != NULL) {
		*usable = ok;
	}

	return true;
}

