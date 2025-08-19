#include <kernel.h>

extern uint8_t* aligned_read_buf;

/* Fills a 512-byte ATA IDENTIFY buffer into 'out'. Returns true on success. */
bool ahci_identify_page(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out) {
	if (!out) {
		return false;
	}

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, false, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, 512, true);

	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_IDENTIFY, 0);

	if (!issue_and_wait(port, slot, "identify")) {
		return false;
	}

	memcpy(out, aligned_read_buf, 512);
	return true;
}
