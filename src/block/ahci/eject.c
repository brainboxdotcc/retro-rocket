#include <kernel.h>
#include <scsi.h>

/* Ejects removable/ejectable media via ATAPI: PREVENT/ALLOW(6)->START STOP UNIT(6). */
bool atapi_eject(ahci_hba_port_t *port, ahci_hba_mem_t *abar) {
	/* 1) Allow medium removal (unlock) */
	{
		GET_SLOT(port, abar);

		ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 0);
		uint64_t ctba = ((uint64_t)cmdheader->command_table_base_lower) | ((uint64_t)cmdheader->command_table_base_upper << 32);
		ahci_hba_cmd_tbl_t* cmdtbl = (ahci_hba_cmd_tbl_t*)(uintptr_t)ctba;

		/* Clear only CFIS+ATAPI+RSV (no PRDT region as PRDTL=0) */
		memset(cmdtbl, 0, offsetof(ahci_hba_cmd_tbl_t, prdt_entry));

		setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

		memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
		scsi_cdb_prevent_allow6* cdb = (scsi_cdb_prevent_allow6*)cmdtbl->atapi_command;
		cdb->opcode  = SCSI_OP_PREVENT_ALLOW_6;
		cdb->prevent = 0; /* allow */

		if (!issue_and_wait(port, slot, "allow medium removal")) {
			return false;
		}
	}

	/* 2) Eject tray: LOEJ=1, START=0, IMMED=1 */
	{
		GET_SLOT(port, abar);

		ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 0);
		uint64_t ctba = ((uint64_t)cmdheader->command_table_base_lower) | ((uint64_t)cmdheader->command_table_base_upper << 32);
		ahci_hba_cmd_tbl_t* cmdtbl = (ahci_hba_cmd_tbl_t*)(uintptr_t)ctba;

		memset(cmdtbl, 0, offsetof(ahci_hba_cmd_tbl_t, prdt_entry));

		setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

		memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
		scsi_cdb_start_stop_unit6* cdb = (scsi_cdb_start_stop_unit6*)cmdtbl->atapi_command;
		cdb->opcode = SCSI_OP_START_STOP_UNIT_6;
		cdb->immed  = 1;          /* safe to set; device may complete early */
		cdb->power  = 0x02;       /* LOEJ=1, START=0 */

		if (!issue_and_wait(port, slot, "eject")) {
			return false;
		}
	}

	return true;
}
