#include <kernel.h>
#include <scsi.h>

extern uint8_t* aligned_read_buf;

/* Issues ATAPI INQUIRY (0x12). Returns true on success. */
bool atapi_enquiry(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out, uint8_t len) {
	if (!out || len == 0) {
		return false;
	}

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, len, true);

	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

	memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
	scsi_cdb_inquiry6* cdb = (scsi_cdb_inquiry6*)cmdtbl->atapi_command;
	cdb->opcode = ATAPI_CMD_INQUIRY;
	cdb->alloc_len = len;

	if (!issue_and_wait(port, slot, "inquiry")) {
		return false;
	}

	memcpy(out, aligned_read_buf, len);
	return true;
}
