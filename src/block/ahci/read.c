#include <kernel.h>
#include <scsi.h>

extern uint8_t* aligned_read_buf;

bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar) {

	if (count > 128) {
		preboot_fail("ahci_read > 128 clusters!");
	}
	uint32_t o_count = count;

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, false, ((count - 1) >> 4) + 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	uint8_t *rd_buf = aligned_read_buf;

	int i = 0;
	for (i = 0; i < cmdheader->prdt_length_entries - 1; i++) {
		fill_prdt(cmdtbl, i, rd_buf, 16 * HDD_SECTOR_SIZE, true);
		rd_buf += 16 * HDD_SECTOR_SIZE;
		count -= 16;
	}
	fill_prdt(cmdtbl, i, rd_buf, count * HDD_SECTOR_SIZE, true);

	ahci_fis_reg_h2d_t* cmdfis = setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_READ_DMA_EX, 0);
	fill_reg_h2c(cmdfis, start, o_count);

	if (!issue_and_wait(port, slot, "read disk")) {
		return false;
	}

	if (!buf) {
		return false;
	}
	memcpy(buf, aligned_read_buf, (o_count * HDD_SECTOR_SIZE));

	add_random_entropy(SAMPLE_FROM_BUFFER(aligned_read_buf));

	return true;
}

uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar) {
	uint8_t id_page[512] = {0};

	if (!ahci_identify_page(port, abar, id_page)) {
		return 0;
	}

	/* Prefer 48-bit MAX LBA if present, else fall back to 28-bit. */
	uint64_t size = ((uint64_t*)(id_page + ATA_IDENT_MAX_LBA_EXT))[0] & 0xFFFFFFFFFFFull;
	if (size == 0) {
		size = ((uint64_t*)(id_page + ATA_IDENT_MAX_LBA))[0] & 0xFFFFFFFFFFFFull;
	}

	return size;
}

bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar) {
	size_t sector_size = ATAPI_SECTOR_SIZE;
	size_t total_bytes = count * sector_size;

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, count * ATAPI_SECTOR_SIZE, true);

	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

	memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
	scsi_cdb_read12* cdb = (scsi_cdb_read12*)cmdtbl->atapi_command;
	cdb->opcode = ATAPI_CMD_READ12;
	cdb->lba_be = htonl((uint32_t)start);
	cdb->xfer_be = htonl(count);

	if (!issue_and_wait(port, slot, "atapi read")) {
		return atapi_handle_check_condition(port, abar, "atapi read");
	}

	memcpy(buf, aligned_read_buf, total_bytes);
	add_random_entropy(SAMPLE_FROM_BUFFER(aligned_read_buf));

	return true;
}

