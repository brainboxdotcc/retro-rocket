#include <kernel.h>

extern uint8_t* aligned_read_buf;

/// @brief Extract TRIM/DSM capability flags from IDENTIFY DEVICE.
ahci_trim_caps ahci_probe_trim_caps(const uint16_t *id_words) {
	ahci_trim_caps caps = {0};

	if (!id_words) {
		return caps;
	}

	uint16_t w69  = id_words[69];
	uint16_t w105 = id_words[105];
	uint16_t w169 = id_words[169];

	caps.has_trim = (w169 & 0x0001) != 0;
	caps.drat = (w69 & 0x4000) != 0;
	caps.rzat = (w69 & 0x0020) != 0;
	caps.max_dsm_blocks = w105 != 0 ? w105 : 1;

	return caps;
}

/// @brief Trim a single contiguous LBA range. No batching/merging of disjoint ranges.
/// @return true on success, or if TRIM unsupported (no-op); false on submission error.
bool ahci_trim_one_range(ahci_hba_port_t *port, ahci_hba_mem_t *abar, const ahci_trim_caps *caps, uint64_t lba, uint32_t sectors) {
	if (sectors == 0) {
		return true;
	}
	if (!caps || !caps->has_trim) {
		/* No-op on devices without TRIM so callers need no special cases. */
		return true;
	}

	/* One DSM block is 512 bytes, containing 64 entries of 8 bytes.
	 * We only use a single entry per command here. Sector-count field is 16-bit.
	 */
	const uint32_t dsm_block_bytes = 512;
	const uint16_t max_entry_sectors = 65535;

	while (sectors != 0) {
		uint16_t chunk = (sectors > max_entry_sectors) ? max_entry_sectors : (uint16_t)sectors;

		memset(aligned_read_buf, 0, dsm_block_bytes);
		/* 48-bit LBA, little-endian, bytes 0..5 */
		aligned_read_buf[0] = (uint8_t)(lba >> 0);
		aligned_read_buf[1] = (uint8_t)(lba >> 8);
		aligned_read_buf[2] = (uint8_t)(lba >> 16);
		aligned_read_buf[3] = (uint8_t)(lba >> 24);
		aligned_read_buf[4] = (uint8_t)(lba >> 32);
		aligned_read_buf[5] = (uint8_t)(lba >> 40);
		/* 16-bit sector count, little-endian, bytes 6..7 */
		aligned_read_buf[6] = (uint8_t)(chunk & 0xff);
		aligned_read_buf[7] = (uint8_t)(chunk >> 8);

		GET_SLOT(port, abar);

		ahci_hba_cmd_header_t *command_header = get_cmdheader_for_slot(port, slot, true, false, 1);
		ahci_hba_cmd_tbl_t    *cmdtbl  = get_and_clear_cmdtbl(command_header);

		fill_prdt(cmdtbl, 0, aligned_read_buf, dsm_block_bytes, false);

		ahci_fis_reg_h2d_t *cfis = setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_DATA_SET_MANAGEMENT, 0x01 /* TRIM */);
		cfis->count_low = 1;
		cfis->count_high = 0;

		if (!issue_and_wait(port, abar, slot)) {
			return atapi_handle_check_condition(port, abar, "trim");
		}

		lba += (uint64_t)chunk;
		sectors -= (uint32_t)chunk;
	}

	return true;
}

