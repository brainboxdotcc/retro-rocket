#include <kernel.h>
#include <scsi.h>

typedef struct scsi_sense_map_entry_t {
	scsi_sense_key_t sense_key;
	scsi_additional_sense_code_t asc;
	scsi_additional_sense_code_qualifier_t ascq;
	fs_error_t error;
} scsi_sense_map_entry_t;

static const scsi_sense_map_entry_t scsi_sense_map[] = {
	{ SCSI_SK_NO_SENSE,		SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_ATAPI_CHECK },
	{ SCSI_SK_RECOVERED_ERROR,	SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_NO_ERROR },
	{ SCSI_SK_NOT_READY,		SCSI_ASC_MEDIUM_NOT_PRESENT,	SCSI_ASCQ_MEDIUM_NOT_PRESENT,	FS_ERR_NO_MEDIA },
	{ SCSI_SK_NOT_READY,		SCSI_ASC_LUN_NOT_READY,		SCSI_ASCQ_BECOMING_READY,	FS_ERR_TIMEOUT },
	{ SCSI_SK_NOT_READY,		SCSI_ASC_LUN_NOT_READY,		SCSI_ASCQ_INIT_REQUIRED,	FS_ERR_TIMEOUT },
	{ SCSI_SK_NOT_READY,		SCSI_ASC_LUN_NOT_READY,		SCSI_ASCQ_MANUAL_INTERVENTION,	FS_ERR_TIMEOUT },
	{ SCSI_SK_NOT_READY,		SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_TIMEOUT },
	{ SCSI_SK_UNIT_ATTENTION,	SCSI_ASC_MEDIA_CHANGED,		SCSI_ASCQ_MEDIA_CHANGED,	FS_ERR_ATAPI_CHECK },
	{ SCSI_SK_UNIT_ATTENTION,	SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_ATAPI_CHECK },
	{ SCSI_SK_DATA_PROTECT,		SCSI_ASC_WRITE_PROTECTED,	SCSI_ASCQ_WRITE_PROTECTED,	FS_ERR_DEVICE_LOCKED },
	{ SCSI_SK_DATA_PROTECT,		SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_DEVICE_LOCKED },
	{ SCSI_SK_ILLEGAL_REQUEST,	SCSI_ASC_INVALID_FIELD,		SCSI_ASCQ_ANY,			FS_ERR_DEVICE_UNSUPPORTED },
	{ SCSI_SK_ILLEGAL_REQUEST,	SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_DEVICE_UNSUPPORTED },
	{ SCSI_SK_MEDIUM_ERROR,		SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_HW_ERROR },
	{ SCSI_SK_HARDWARE_ERROR,	SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_HW_ERROR },
	{ SCSI_SK_ABORTED_COMMAND,	SCSI_ASC_ANY,			SCSI_ASCQ_ANY,			FS_ERR_HW_ERROR },
};

int ahci_classify_error(ahci_hba_port_t* port) {

	uint32_t is   = port->interrupt_status;
	uint32_t tfd  = port->task_file_data;
	uint32_t sig  = port->signature;

	if ((is & HBA_PxIS_HBFS) != 0) {
		return FS_ERR_AHCI_HOSTBUS_FATAL;
	}

	if ((is & HBA_PxIS_HBDS) != 0) {
		return FS_ERR_AHCI_HOSTBUS_DATA;
	}

	if ((is & HBA_PxIS_IFS) != 0) {
		return FS_ERR_AHCI_LINK_ERROR;
	}

	if ((is & HBA_PxIS_TFES) != 0) {
		if (sig == SATA_SIG_ATAPI) {
			/* ATAPI: CHECK CONDITION - higher layer should REQUEST SENSE. */
			return FS_ERR_ATAPI_CHECK;
		}

		/* ATA taskfile error: pick broad, actionable buckets. */
		uint8_t ata_err = (uint8_t)((tfd >> 8) & 0xff);

		if ((ata_err & ATA_ERR_ABRT) != 0) {
			return FS_ERR_DEVICE_UNSUPPORTED;
		} else if ((ata_err & ATA_ERR_UNC) != 0) {
			return FS_ERR_DATA_ERROR;
		} else if ((ata_err & ATA_ERR_IDNF) != 0) {
			return FS_ERR_OUT_OF_BOUNDS;
		} else if ((ata_err & ATA_ERR_MC) != 0) {
			return FS_ERR_AHCI_TASKFILE;
		} else if ((ata_err & ATA_ERR_ICRC) != 0) {
			return FS_ERR_AHCI_LINK_ERROR;
		}
		/* Fallback for other ATA error bits (TK0NF, AMNF, etc.). */
		return FS_ERR_AHCI_TASKFILE;
	}

	/* If we reached here, we took an error path without the usual flags. */
	return FS_ERR_HW_ERROR;
}

int scsi_map_sense_to_fs_error(scsi_sense_key_t sense_key, scsi_additional_sense_code_t additional_sense_code, scsi_additional_sense_code_qualifier_t additional_sense_code_qualifier) {
	for (size_t i = 0; i < (sizeof(scsi_sense_map) / sizeof(scsi_sense_map[0])); i++) {
		const scsi_sense_map_entry_t *e = &scsi_sense_map[i];
		if (e->sense_key == sense_key && (e->asc  == SCSI_ASC_ANY || e->asc == additional_sense_code) && (e->ascq == SCSI_ASCQ_ANY || e->ascq == additional_sense_code_qualifier)) {
			return e->error;
		}
	}
	return FS_ERR_HW_ERROR;
}

bool atapi_handle_check_condition(ahci_hba_port_t* port, ahci_hba_mem_t* abar, const char* function) {
	uint8_t sense[18];
	if (atapi_request_sense6(port, abar, sense, sizeof(sense))) {
		scsi_sense_key_t sense_key;
		scsi_additional_sense_code_t additional_sense_code;
		scsi_additional_sense_code_qualifier_t additional_sense_code_qualifier;

		scsi_extract_fixed_sense(sense, &sense_key, &additional_sense_code, &additional_sense_code_qualifier);
		fs_error_t fe = scsi_map_sense_to_fs_error(sense_key, additional_sense_code, additional_sense_code_qualifier);

		dprintf(
			"ATAPI %s failed: %s (SK=%02x ASC=%02x ASCQ=%02x)\n",
			function,
			fs_strerror(fe),
			sense_key, additional_sense_code, additional_sense_code_qualifier
		);
	} else {
		dprintf("ATAPI %s failed: %s (no sense data)\n", function, fs_strerror(FS_ERR_TIMEOUT));
	}

	return false;
}