#include "kernel.h"

static uint8_t* aligned_read_buf = NULL;
static uint8_t* aligned_write_buf = NULL;

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

void port_rebase(ahci_hba_port_t* port, int portno)
{
	stop_cmd(port);
 
	port->command_list_base_addr_lower = (kmalloc_low(0x100000) & 0xFFFFFFFFFFFFF000) + 0x1000;
	port->command_list_base_addr_upper = 0;
	memset((void*)((uint64_t)port->command_list_base_addr_lower), 0, 1024);

	port->fis_base_addr_lower = port->command_list_base_addr_lower + (32 * 1024) + (portno << 8);
	port->fis_base_addr_upper = 0;
	memset((void*)((uint64_t)port->fis_base_addr_lower), 0, 256);

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)((uint64_t)port->command_list_base_addr_lower);
	for (int i = 0; i < 32; i++) {
		cmdheader[i].prdt_length_entries = 8;
		cmdheader[i].command_table_base_lower = port->command_list_base_addr_lower + (40 * 1024) + (portno << 13) + (i << 8);
		cmdheader[i].command_table_base_upper = 0;
		memset((void*)(uint64_t)cmdheader[i].command_table_base_lower, 0, 256);
	}
	start_cmd(port);
}

static void trim_trailing_spaces(char *s) {
	size_t n = strlen(s);
	while (n > 0) {
		if (s[n - 1] == ' ') {
			s[n - 1] = '\0';
			n--;
		} else {
			break;
		}
	}
}

static void build_atapi_label(storage_device_t *sd, const uint8_t *inq) {
	char vendor[16] = {0};
	char product[32] = {0};
	char rev[16] = {0};

	memcpy(vendor,  inq + 8,  8);
	memcpy(product, inq + 16, 16);
	memcpy(rev,     inq + 32, 4);

	trim_trailing_spaces(vendor);
	trim_trailing_spaces(product);
	trim_trailing_spaces(rev);

	if (rev[0] != '\0') {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "%s %s - %s (Optical)", vendor, product, rev);
	} else {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "%s %s (Optical)", vendor, product);
	}

	sd->ui.is_optical = true;
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
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->command_list_base_addr_lower;
	cmdheader += slot;
	cmdheader->command_fis_length = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->write = write ? 1 : 0;
	cmdheader->atapi = atapi ? 1 : 0;
	cmdheader->prdt_length_entries = prdtls;
	return cmdheader;
}

ahci_hba_cmd_tbl_t* get_and_clear_cmdtbl(ahci_hba_cmd_header_t* cmdheader) {
	ahci_hba_cmd_tbl_t *cmdtbl = (ahci_hba_cmd_tbl_t*)((uint64_t)cmdheader->command_table_base_lower);
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

bool wait_for_completion(ahci_hba_port_t* port, uint8_t slot, const char* function) {
	while (true) {
		if ((port->command_issue & (1 << slot)) == 0) {
			break;
		}
		if (port->interrupt_status & HBA_PxIS_TFES) {
			dprintf("Disk error [%s]\n", function);
			return false;
		}
	}

	if (port->interrupt_status & HBA_PxIS_TFES) {
		dprintf("Disk error [%s]\n", function);
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

/* Issues ATAPI INQUIRY (0x12). 'len' should be <= 255 (6-byte CDB limit). Returns true on success. */
bool atapi_enquiry(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out, uint8_t len)
{
	if (!out || len == 0) {
		return false;
	}

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, len, true);

	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

	memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
	cmdtbl->atapi_command[0] = ATAPI_CMD_INQUIRY;	/* INQUIRY (6) */
	cmdtbl->atapi_command[4] = len;			/* Allocation length (1 byte for 6-byte CDB) */

	if (!issue_and_wait(port, slot, "inquiry")) {
		return false;
	}

	memcpy(out, aligned_read_buf, len);
	return true;
}


void probe_port(ahci_hba_mem_t *abar, pci_dev_t dev)
{
	uint32_t port_index = abar->ports_implemented;
	int i = 0;
	while (i < 32) {
		if (port_index & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				port_rebase(&abar->ports[i], i);

				// Enable interrupts for this port
				abar->ports[i].interrupt_enable = 0xFFFFFFFF;
				abar->global_host_control |= (1 << 1);  // global IE


				storage_device_t* sd = kmalloc(sizeof(storage_device_t));
				if (!sd) {
					return;
				}
				sd->opaque1 = i;
				sd->opaque2 = (void*)abar;
				sd->cache = NULL;
				sd->blockread = storage_device_ahci_block_read;
				sd->blockwrite = storage_device_ahci_block_write;
				sd->size = dt == AHCI_DEV_SATA ? ahci_read_size(&abar->ports[i], abar) : SIZE_MAX;
				make_unique_device_name(dt == AHCI_DEV_SATA ? "hd" : "cd", sd->name, sizeof(sd->name));
				sd->block_size = dt == AHCI_DEV_SATA ? HDD_SECTOR_SIZE : ATAPI_SECTOR_SIZE;
				/* Build end-user label */
				sd->ui.label[0] = 0;
				sd->ui.is_optical = false;
				if (dt == AHCI_DEV_SATA) {
					uint8_t id_page[512] = {0};
					if (ahci_identify_page(&abar->ports[i], abar, id_page)) {
						build_sata_label(sd, id_page);
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
				kprintf("%s storage, Port #%d: %s\n", dt == AHCI_DEV_SATA ? "SATA" : "ATAPI", i, sd->ui.label);
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

bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar) {

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

	add_random_entropy(*buf);
	return true;
}

uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar) {
	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, false, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, 512, true);
	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_IDENTIFY, 0);

	if (!issue_and_wait(port, slot, "read size")) {
		return 0;
	}

	uint64_t size = ((uint64_t*)(aligned_read_buf + ATA_IDENT_MAX_LBA_EXT)) [0] & 0xFFFFFFFFFFFull;
	if (size == 0) {
		size = ((uint64_t*)(aligned_read_buf + ATA_IDENT_MAX_LBA))[0] & (uint64_t)0xFFFFFFFFFFFFull;
	}

	return size;
}

bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar) {
	size_t sector_size = ATAPI_SECTOR_SIZE;
	size_t total_bytes = count * sector_size;

	GET_SLOT(port, abar);

	ahci_hba_cmd_header_t* cmdheader = get_cmdheader_for_slot(port, slot, false, true, 1);
	ahci_hba_cmd_tbl_t* cmdtbl = get_and_clear_cmdtbl(cmdheader);

	fill_prdt(cmdtbl, 0, aligned_read_buf, count * ATAPI_SECTOR_SIZE, true);

	setup_reg_h2d(cmdtbl, FIS_TYPE_REG_H2D, ATA_CMD_PACKET, 5);

	memset(cmdtbl->atapi_command, 0, sizeof(cmdtbl->atapi_command));
	cmdtbl->atapi_command[0] = ATAPI_CMD_READ12;
	cmdtbl->atapi_command[2] = (uint8_t)((start >> 24) & 0xff);
	cmdtbl->atapi_command[3] = (uint8_t)((start >> 16) & 0xff);
	cmdtbl->atapi_command[4] = (uint8_t)((start >> 8) & 0xff);
	cmdtbl->atapi_command[5] = (uint8_t)((start >> 0) & 0xff);
	cmdtbl->atapi_command[6] = (count >> 24) & 0xFF;
	cmdtbl->atapi_command[7] = (count >> 16) & 0xFF;
	cmdtbl->atapi_command[8] = (count >> 8) & 0xFF;
	cmdtbl->atapi_command[9] = (count >> 0) & 0xFF;

	if (!issue_and_wait(port, slot, "atapi read")) {
		return false;
	}

	memcpy(buf, aligned_read_buf, total_bytes);
	add_random_entropy(*buf);

	return true;
}

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

void setup_aligned_buffers() {
	uint64_t bounce_size = HDD_SECTOR_SIZE * 128 + 0x1000;
	aligned_read_buf = ((uintptr_t)kmalloc_low(bounce_size) + 0xFFF) & ~0xFFF;
	aligned_write_buf = ((uintptr_t)kmalloc_low(bounce_size) + 0xFFF) & ~0xFFF;
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

	setup_aligned_buffers();

	uint32_t irq_num = pci_setup_interrupt("AHCI", ahci_device, logical_cpu_id(), ahci_handler, ahci_base);
	dprintf("AHCI base MMIO: %lx INT %d\n", ahci_base, irq_num);

	probe_port((ahci_hba_mem_t*)ahci_base, ahci_device);
}
