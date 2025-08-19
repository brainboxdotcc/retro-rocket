#include <kernel.h>

extern uint8_t* aligned_write_buf;

bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar) {
	uint32_t o_count = count;
	GET_SLOT(port, abar);

	memcpy(aligned_write_buf, buf, o_count * HDD_SECTOR_SIZE);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, true, false, ((o_count-1)>>4) + 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);
	uint8_t* wr_buf = aligned_write_buf;
	int i;

	for (i = 0; i< cmdheader->prdt_length_entries - 1; i++) {
		fill_prdt(cmdtbl, i, wr_buf, 16 * HDD_SECTOR_SIZE, true);
		wr_buf += 16 * HDD_SECTOR_SIZE;
		count -= 16;
	}
	fill_prdt(cmdtbl, i, wr_buf, count * HDD_SECTOR_SIZE, true);

	ahci_fis_reg_h2d_t* cmdfis = setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_WRITE_DMA_EX, 0);
	fill_reg_h2c(cmdfis, start, o_count);

	if (!issue_and_wait(port, slot, "write disk")) {
		return false;
	}

	return true;
}

