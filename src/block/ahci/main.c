#include <kernel.h>
#include <scsi.h>

uint8_t* aligned_read_buf = NULL;
uint8_t* aligned_write_buf = NULL;

void ahci_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void* opaque)
{
	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)opaque;
	uint32_t interrupt_status = abar->interrupt_status;
	for (int i = 0; i < 32; i++) {
		if (interrupt_status & (1u << i)) {
			uint32_t pis = abar->ports[i].interrupt_status;
			abar->ports[i].interrupt_status = pis; // ack by writing back
			//dprintf("AHCI IRQ: port %d, PxIS=%08x\n", i, pis);
		}
	}
}

static inline bool ahci_hba_supports_64b(const ahci_hba_mem_t* abar) {
	return (abar->host_capabilities & HBA_CAP_S64A) != 0;
}

void* allocate_ahci(const ahci_hba_mem_t* abar, size_t size) {
	return ahci_hba_supports_64b(abar) ? kmalloc(size) : (void*)kmalloc_low(size);
}

int check_type(ahci_hba_port_t* port)
{
	uint32_t ssts = port->sata_status;
 	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
 
	if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE) {
		return AHCI_DEV_NULL;
	}

	switch (port->signature) {
		case SATA_SIG_ATAPI:
			return AHCI_DEV_SATAPI;
		case SATA_SIG_SEMB:
			return AHCI_DEV_SEMB;
		case SATA_SIG_PM:
			return AHCI_DEV_PM;
		default:
			return AHCI_DEV_SATA;
	}
}

void start_cmd(ahci_hba_port_t *port)
{
	while (port->cmd & HBA_PxCMD_CR) {}
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}
 
void stop_cmd(ahci_hba_port_t *port)
{
	port->cmd &= ~HBA_PxCMD_ST;
	port->cmd &= ~HBA_PxCMD_FRE;
	while (true) {
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

void port_rebase(ahci_hba_port_t* port, int portno, ahci_hba_mem_t* abar)
{
	stop_cmd(port);

	uint64_t clb = (((uint64_t)allocate_ahci(abar, 0x100000)) & 0xFFFFFFFFFFFFF000) + 0x1000;
	port->command_list_base_addr_lower = (uint32_t)clb;
	port->command_list_base_addr_upper = (uint32_t)(clb >> 32);
	memset((void*)clb, 0, 1024);

	uint64_t fb = clb + (32 * 1024) + ((uint64_t)portno << 8);
	port->fis_base_addr_lower = (uint32_t)fb;
	port->fis_base_addr_upper = (uint32_t)(fb >> 32);
	memset((void*)fb, 0, 256);

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uintptr_t)clb;
	for (int i = 0; i < 32; i++) {
		cmdheader[i].prdt_length_entries = 8;
		uint64_t ctba = clb + (40 * 1024) + ((uint64_t)portno << 13) + ((uint64_t)i << 8);
		cmdheader[i].command_table_base_lower = (uint32_t)ctba;
		cmdheader[i].command_table_base_upper = (uint32_t)(ctba >> 32);
		memset((void*)(uintptr_t)ctba, 0, 256);
	}
	start_cmd(port);
}

bool wait_for_ready(ahci_hba_port_t* port) {
	int spin = 0;
	while ((port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		return false;
	}
	return true;
}

void fill_prdt(ahci_hba_cmd_tbl_t* cmdtbl, size_t index, void* address, uint32_t byte_count, bool interrupt) {
	cmdtbl->prdt_entry[index].data_address_lower = (uint32_t)((uint64_t)address & 0xffffffff);
	cmdtbl->prdt_entry[index].data_address_upper = (uint32_t)(((uint64_t)(address) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[index].byte_count = byte_count - 1;
	cmdtbl->prdt_entry[index].interrupt = interrupt ? 1 : 0;
}

ahci_hba_cmd_header_t* get_cmdheader_for_slot(ahci_hba_port_t* port, size_t slot, bool write, bool atapi, uint16_t prdtls) {
	uint64_t clb = ((uint64_t)port->command_list_base_addr_lower) | ((uint64_t)port->command_list_base_addr_upper << 32);
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)clb;
	cmdheader += slot;
	cmdheader->command_fis_length = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->write = write ? 1 : 0;
	cmdheader->atapi = atapi ? 1 : 0;
	cmdheader->prdt_length_entries = prdtls;
	return cmdheader;
}

ahci_hba_cmd_tbl_t* get_and_clear_cmdtbl(ahci_hba_cmd_header_t* cmdheader) {
	uint64_t ctba = ((uint64_t)cmdheader->command_table_base_lower) | ((uint64_t)cmdheader->command_table_base_upper << 32);
	ahci_hba_cmd_tbl_t *cmdtbl = (ahci_hba_cmd_tbl_t*)ctba;
	memset(cmdtbl, 0, sizeof(ahci_hba_cmd_tbl_t) + (cmdheader->prdt_length_entries - 1) * sizeof(ahci_hba_prdt_entry_t));
	return cmdtbl;
}

ahci_fis_reg_h2d_t* setup_reg_h2d(ahci_hba_cmd_tbl_t* cmdtbl, uint8_t type, uint8_t command, uint8_t feature_low) {
	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->command_fis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));
	cmdfis->fis_type = type;
	cmdfis->command_or_control = 1;
	cmdfis->command = command;
	if (feature_low) {
		cmdfis->feature_low = feature_low;
	}
	return cmdfis;
}

void fill_reg_h2c(ahci_fis_reg_h2d_t* cmdfis, uint64_t start, uint16_t count) {

	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = ((start & 0xFFFFFFFF00000000) >> 32);

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1 << 6;  // LBA mode

	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);

	cmdfis->count_low = count & 0xFF;
	cmdfis->count_high = (count >> 8) & 0xFF;

}

void issue_command_to_slot(ahci_hba_port_t *port, uint8_t slot) {
	port->command_issue = 1 << slot;
}

bool is_error(ahci_hba_port_t* port, const char* function) {
	if (port->interrupt_status & HBA_PxIS_TFES) {
		fs_error_t fe = ahci_classify_error(port);
		dprintf("Disk error [%s]: %s [IS=%08x TFD=%08x SERR=%08x]\n", function, fs_strerror(fe), port->interrupt_status, port->task_file_data, port->sata_error);
		return true;
	}
	return false;
}

bool wait_for_completion(ahci_hba_port_t* port, uint8_t slot, const char* function) {
	while (true) {
		if ((port->command_issue & (1 << slot)) == 0) {
			break;
		}
		if (is_error(port, function)) {
			return false;
		}
	}

	if (is_error(port, function)) {
		return false;
	}
	return true;
}

bool issue_and_wait(ahci_hba_port_t* port, uint8_t slot, const char* function) {
	if (!wait_for_ready(port)) {
		dprintf("Port hung [%s]\n", function);
		return false;
	}
	issue_command_to_slot(port, slot);
	return wait_for_completion(port, slot, function);
}

void probe_port(ahci_hba_mem_t *abar)
{
	uint32_t port_index = abar->ports_implemented;
	int i = 0;
	while (i < 32) {
		if (port_index & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				port_rebase(&abar->ports[i], i, abar);

				// Enable interrupts for this port
				abar->ports[i].interrupt_enable = 0xFFFFFFFF;
				abar->global_host_control |= (1 << 1);  // global IE

				storage_device_t* sd = kmalloc(sizeof(storage_device_t));
				if (!sd) {
					return;
				}
				sd->opaque1 = i;
				sd->opaque2 = (void*)abar;
				sd->opaque3 = NULL;
				sd->cache = NULL;
				sd->blockread = storage_device_ahci_block_read;
				sd->blockwrite = storage_device_ahci_block_write;
				sd->blockclear = storage_device_ahci_block_clear;
				sd->size = dt == AHCI_DEV_SATA ? ahci_read_size(&abar->ports[i], abar) : SIZE_MAX;
				make_unique_device_name(dt == AHCI_DEV_SATA ? "hd" : "cd", sd->name, sizeof(sd->name));
				sd->block_size = dt == AHCI_DEV_SATA ? HDD_SECTOR_SIZE : ATAPI_SECTOR_SIZE;
				/* Build end-user label */
				sd->ui.label[0] = 0;
				sd->ui.is_optical = false;
				bool has_trim = false;
				if (dt == AHCI_DEV_SATA) {
					uint8_t id_page[512] = {0};
					if (ahci_identify_page(&abar->ports[i], abar, id_page)) {
						build_sata_label(sd, id_page);
						ahci_trim_caps* caps = kmalloc(sizeof(ahci_trim_caps));
						if (caps) {
							*caps = ahci_probe_trim_caps((uint16_t *) id_page);
							sd->opaque3 = caps;
							has_trim = caps->has_trim;
						}
					} else {
						char size_str[24] = {0};
						humanise_capacity(size_str, sizeof(size_str), sd->size * sd->block_size);
						snprintf(sd->ui.label, sizeof(sd->ui.label), "ATA Device - %s", size_str);
					}
				} else {
					uint8_t inq[96] = {0};
					if (atapi_enquiry(&abar->ports[i], abar, inq, sizeof(inq))) {
						build_atapi_label(sd, inq);
					} else {
						snprintf(sd->ui.label, sizeof(sd->ui.label), "ATAPI Device (Optical)");
						sd->ui.is_optical = true;
					}
				}
				kprintf("%s storage, Port #%d: %s (%d bit%s)\n", dt == AHCI_DEV_SATA ? "SATA" : "ATAPI", i, sd->ui.label, ahci_hba_supports_64b(abar) ? 64 : 32, has_trim ? ", TRIM" : "");
				register_storage_device(sd);
				storage_enable_cache(sd);
			}
		}
		port_index >>= 1;
		i ++;
	}
}

int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar) {
	// If not set in SACT and CI, the slot is free
	port->interrupt_status = (uint32_t)-1;
	uint32_t slots = (port->sata_active | port->command_issue);
	int cmdslots = (abar->host_capabilities & 0x0f00) >> 8;
	for (int i=0; i<cmdslots; i++) {
		if ((slots&1) == 0) {
			return i;
		}
		slots >>= 1;
	}
	dprintf("Cannot find free command list entry\n");
	return -1;
}

void setup_aligned_buffers(ahci_hba_mem_t* abar) {
	uint64_t bounce_size = HDD_SECTOR_SIZE * 128 + 0x1000;
	aligned_read_buf  = ((uintptr_t)(allocate_ahci(abar, bounce_size)) + 0xFFF) & ~0xFFF;
	aligned_write_buf = ((uintptr_t)(allocate_ahci(abar, bounce_size)) + 0xFFF) & ~0xFFF;
}

void init_ahci() {

	pci_dev_t ahci_device = pci_get_device(0, 0, 0x0106);
	if (!ahci_device.bits) {
		dprintf("No AHCI devices found\n");
		return;
	}

	uint64_t ahci_base = pci_read(ahci_device, PCI_BAR5);
	if (pci_bar_type(ahci_base) != PCI_BAR_TYPE_MEMORY) {
		dprintf("AHCI device does not have a memory BAR5\n");
		return;
	}

	ahci_base = pci_mem_base(ahci_base);
	setup_aligned_buffers((ahci_hba_mem_t*)ahci_base);

	uint32_t irq_num = pci_setup_interrupt("AHCI", ahci_device, logical_cpu_id(), ahci_handler, ahci_base);
	dprintf("AHCI base MMIO: %lx INT %d\n", ahci_base, irq_num);

	probe_port((ahci_hba_mem_t*)ahci_base);
}
