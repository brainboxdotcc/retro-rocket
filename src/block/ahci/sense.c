#include <kernel.h>
#include <scsi.h>

extern uint8_t* aligned_read_buf;

/**
 * @brief Issue ATAPI REQUEST SENSE (6) and copy sense data to 'out'.
 * Expects 'out_len' >= 18 for standard fixed-format sense.
 * Returns true on success (command accepted and data DMA'd), false on failure.
 */
bool atapi_request_sense6(ahci_hba_port_t* port, ahci_hba_mem_t* abar, uint8_t* out, uint8_t out_len) {
	if (!out || out_len == 0) {
		return false;
	}

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, out_len, true);
	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

	memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
	scsi_cdb_request_sense6* cdb = (scsi_cdb_request_sense6*)cmdtbl->atapi_command;
	cdb->opcode    = SCSI_OP_REQUEST_SENSE_6;
	cdb->alloc_len = out_len;

	if (!issue_and_wait(port, slot, "request sense")) {
		return false;
	}

	memcpy(out, aligned_read_buf, out_len);
	return true;
}

void scsi_extract_fixed_sense(const uint8_t* buf, scsi_sense_key_t* sense_key, scsi_additional_sense_code_t* additional_sense_code, scsi_additional_sense_code_qualifier_t* additional_sense_code_qualifier) {
	*sense_key   = (uint8_t)(buf[2] & 0x0f);
	*additional_sense_code  = buf[12];
	*additional_sense_code_qualifier = buf[13];
}