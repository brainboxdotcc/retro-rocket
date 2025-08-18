#include <kernel.h>

void* aligned_buf = NULL;
uint8_t* raw_buf = NULL;
uint8_t* buf = NULL;
static uint8_t* raw_write_buf = NULL;
static uint8_t* aligned_write_buf = NULL;

int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar);
bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);
bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar);
static void build_sata_label(storage_device_t *sd, const uint8_t *id_page);
static void humanise_capacity(char *out, size_t out_len, uint64_t bytes);

void ahci_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void* opaque)
{
	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)opaque;
	uint32_t is = abar->is;
	for (int i = 0; i < 32; i++) {
		if (is & (1u << i)) {
			uint32_t pis = abar->ports[i].is;
			abar->ports[i].is = pis; // ack by writing back
			//dprintf("AHCI IRQ: port %d, PxIS=%08x\n", i, pis);
		}
	}
}

static int check_type(ahci_hba_port_t* port)
{
	uint32_t ssts = port->ssts;
 
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
 
	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;
 
	switch (port->sig) {
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
 
	port->clb = (kmalloc_low(0x100000) & 0xFFFFFFFFFFFFF000) + 0x1000;
	port->clbu = 0;
	memset((void*)((uint64_t)port->clb), 0, 1024);

	port->fb = port->clb + (32 * 1024) + (portno << 8);
	port->fbu = 0;
	memset((void*)((uint64_t)port->fb), 0, 256);

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)((uint64_t)port->clb);
	for (int i = 0; i < 32; i++) {
		cmdheader[i].prdtl = 8;
		cmdheader[i].ctba = port->clb + (40 * 1024) + (portno << 13) + (i << 8);
		cmdheader[i].ctbau = 0;
		memset((void*)(uint64_t)cmdheader[i].ctba, 0, 256);
	}
	start_cmd(port);
}

int storage_device_ahci_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd || !buffer || bytes == 0) {
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	unsigned char* end = buffer + bytes;

	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	const uint32_t max_per_cmd = 16;

	if (check_type(port) == AHCI_DEV_SATAPI) {
		while (sectors > 0) {
			uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
			if (!ahci_atapi_read(port, start, this_xfer, (uint16_t*)buffer, abar)) {
				return false;
			}
			start   += this_xfer;
			buffer  += this_xfer * sd->block_size;
			sectors -= this_xfer;
		}
		return true;
	} else {
		while (sectors > 0) {
			uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
			uint32_t bytes_this = this_xfer * sd->block_size;

			if (buffer + bytes_this > end) {
				uint64_t bytes_left = (uint64_t)(end - buffer);
				this_xfer = bytes_left / sd->block_size;
				if (this_xfer == 0) {
					break;
				}
				bytes_this = this_xfer * sd->block_size;
			}

			if (!ahci_read(port, start, this_xfer, (uint16_t*)buffer, abar)) {
				return 0;
			}

			start   += this_xfer;
			buffer  += bytes_this;
			sectors -= this_xfer;
		}
		return 1;
	}
}

int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	const uint32_t max_per_cmd = 16;
	while (sectors > 0) {
		uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
		if (!ahci_write(port, start, this_xfer, (char*)buffer, abar)) {
			return false;
		}
		start   += this_xfer;
		buffer  += this_xfer * sd->block_size;
		sectors -= this_xfer;
	}

	return true;
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

/* Fills a 512-byte ATA IDENTIFY buffer into 'out'. Returns true on success. */
bool ahci_identify_page(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out)
{
	if (!out) {
		return false;
	}

	port->is = (uint32_t)-1;

	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		return false;
	}

	if (!raw_buf && !buf) {
		raw_buf = (uint8_t*)kmalloc_low(HDD_SECTOR_SIZE + 0x1000);
		buf = (uint8_t*)(((uintptr_t)raw_buf + 0xFFF) & ~0xFFF);
	}

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->w = 0;
	cmdheader->prdtl = 1;

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	cmdtbl->prdt_entry[0].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
	cmdtbl->prdt_entry[0].dbau = (uint32_t)(((uint64_t)buf) >> 32);
	cmdtbl->prdt_entry[0].dbc = 511;
	cmdtbl->prdt_entry[0].i = 1;

	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->command = ATA_CMD_IDENTIFY;

	int spin = 0;
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [identify]\n");
		return false;
	}

	port->ci = 1 << slot;

	while (true) {
		if ((port->ci & (1 << slot)) == 0) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) {
			dprintf("Disk error [identify]\n");
			return false;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		dprintf("Disk error [identify]\n");
		return false;
	}

	memcpy(out, buf, 512);
	return true;
}

/* Issues ATAPI INQUIRY (0x12). 'len' should be <= 255 (6-byte CDB limit). Returns true on success. */
bool atapi_enquiry(ahci_hba_port_t *port, ahci_hba_mem_t *abar, uint8_t *out, uint16_t len)
{
	if (!out || len == 0) {
		return false;
	}
	if (len > 255) {
		len = 255;
	}

	if (!aligned_buf) {
		uint8_t *raw = (uint8_t*)kmalloc_low(ATAPI_SECTOR_SIZE * 16);
		uintptr_t raw_addr = (uintptr_t)raw;
		uintptr_t aligned_addr = (raw_addr + (ATAPI_SECTOR_SIZE - 1)) & ~(ATAPI_SECTOR_SIZE - 1);
		aligned_buf = (void*)aligned_addr;
	}

	port->is = (uint32_t)-1;

	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		dprintf("No free command slots [inquiry]\n");
		return false;
	}

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->w = 0;
	cmdheader->a = 1;
	cmdheader->prdtl = 1;

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	cmdtbl->prdt_entry[0].dba = (uint32_t)((uint64_t)aligned_buf & 0xffffffff);
	cmdtbl->prdt_entry[0].dbau = (uint32_t)(((uint64_t)aligned_buf) >> 32);
	cmdtbl->prdt_entry[0].dbc = len - 1;
	cmdtbl->prdt_entry[0].i = 1;

	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->featurel = 5;                /* match your ATAPI READ path behaviour */
	cmdfis->command = ATA_CMD_PACKET;

	memset(cmdtbl->acmd, 0, sizeof(cmdtbl->acmd));
	cmdtbl->acmd[0] = 0x12;              /* INQUIRY (6) */
	cmdtbl->acmd[1] = 0;                 /* EVPD=0 */
	cmdtbl->acmd[2] = 0;                 /* Page code */
	cmdtbl->acmd[3] = 0;                 /* Reserved */
	cmdtbl->acmd[4] = (uint8_t)len;      /* Allocation length (1 byte for 6-byte CDB) */
	cmdtbl->acmd[5] = 0;                 /* Control */

	int spin = 0;
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [inquiry]\n");
		return false;
	}

	port->ci = 1 << slot;

	while (true) {
		if ((port->ci & (1 << slot)) == 0) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) {
			dprintf("Disk error [inquiry]\n");
			return false;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		dprintf("Disk error [inquiry]\n");
		return false;
	}

	memcpy(out, aligned_buf, len);
	return true;
}


void probe_port(ahci_hba_mem_t *abar, pci_dev_t dev)
{
	uint32_t pi = abar->pi;
	int i = 0;
	while (i < 32) {
		if (pi & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				port_rebase(&abar->ports[i], i);

				// Enable interrupts for this port
				abar->ports[i].ie = 0xFFFFFFFF;
				abar->ghc |= (1 << 1);  // global IE


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
		pi >>= 1;
		i ++;
	}
}

int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar) {
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	int cmdslots = (abar->cap & 0x0f00) >> 8;
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
	static uint8_t *raw_read_buf = NULL;
	static uint8_t *aligned_read_buf = NULL;

	if (count > 128) {
		preboot_fail("ahci_read > 128 clusters!");
	}
	uint32_t o_count = count;

	if (!raw_read_buf) {
		raw_read_buf = (uint8_t*)kmalloc_low(HDD_SECTOR_SIZE * 128 + 0x1000);
		aligned_read_buf = (uint8_t*)(((uintptr_t)raw_read_buf + 0xFFF) & ~0xFFF);
	}

	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = (uint32_t)((start >> 32) & 0xFFFFFFFF);

	port->is = (uint32_t)-1;

	int spin = 0;
	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		return false;
	}

	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size
	cmdheader->w   = 0;   // read
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1; // PRDT entries count (ceil(count/16))

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	uint8_t *rd_buf = aligned_read_buf;

	int i = 0;
	for (i = 0; i < cmdheader->prdtl - 1; i++) {
		cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)rd_buf & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)rd_buf) >> 32);
		cmdtbl->prdt_entry[i].dbc  = 8 * 1024 - 1; // 16 sectors (8KiB)
		cmdtbl->prdt_entry[i].i    = 1;            // interrupt on completion
		rd_buf += 8 * 1024;
		count  -= 16;
	}
	cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)rd_buf & 0xFFFFFFFF);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)rd_buf) >> 32);
	cmdtbl->prdt_entry[i].dbc  = (count * HDD_SECTOR_SIZE) - 1; // exact size
	cmdtbl->prdt_entry[i].i    = 1;

	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c        = 1; // Command
	cmdfis->command  = ATA_CMD_READ_DMA_EX;

	cmdfis->lba0    = (uint8_t) startl;
	cmdfis->lba1    = (uint8_t)(startl >> 8);
	cmdfis->lba2    = (uint8_t)(startl >> 16);
	cmdfis->device  = 1 << 6; // LBA mode

	cmdfis->lba3    = (uint8_t)(startl >> 24);
	cmdfis->lba4    = (uint8_t)(starth);
	cmdfis->lba5    = (uint8_t)(starth >> 8);

	cmdfis->countl  = o_count & 0xFF;
	cmdfis->counth  = (o_count >> 8) & 0xFF;

	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [read] (start %lx count %x)\n", start, o_count);
		return false;
	}

	port->ci = 1 << slot;

	while (true) {
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Read disk error (start %lx count %x)\n", start, o_count);
			return false;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		dprintf("Read disk error (start %lx count %x)\n", start, o_count);
		return false;
	}

	if (!buf) {
		return false;
	}
	memcpy(buf, aligned_read_buf, (o_count * HDD_SECTOR_SIZE));

	add_random_entropy(*buf);
	return true;
}

uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar)
{
	port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		return 0;
	}
 
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = 1;	// PRDT entries count
 
	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(ahci_hba_prdt_entry_t));
 
	if (!raw_buf && !buf) {
		raw_buf = (uint8_t*)kmalloc_low(HDD_SECTOR_SIZE + 0x1000); // extra for alignment
		buf = (uint8_t*)(((uintptr_t)raw_buf + 0xFFF) & ~0xFFF); // 4K align
	}

	// 8K bytes (16 sectors) per PRDT
	// Last entry
	cmdtbl->prdt_entry[0].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
	cmdtbl->prdt_entry[0].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[0].dbc = 511;
	cmdtbl->prdt_entry[0].i = 1;

	// Setup command
	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t)); 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_IDENTIFY; // ATA_CMD_READ_DMA_EX;
 
 	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [read size]\n");
		return 0;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	// Wait for completion
	while (true) {
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1<<slot)) == 0) 
			break;
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Read disk error [read size]\n");
			return 0;
		}
	}
 
	// Check again
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Read disk error [read size]\n");
		return 0;
	}

	uint64_t size = ((uint64_t*)(buf + ATA_IDENT_MAX_LBA_EXT)) [0] & 0xFFFFFFFFFFFull;
	if (size == 0) {
		size = ((uint64_t*)(buf + ATA_IDENT_MAX_LBA))[0] & (uint64_t)0xFFFFFFFFFFFFull;
	}

	return size;
}

bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar)
{
	size_t sector_size = ATAPI_SECTOR_SIZE;
	size_t total_bytes = count * sector_size;

	port->is = (uint32_t)-1;
	int spin = 0;
	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		dprintf("No free command slots\n");
		return false;
	}

	ahci_hba_cmd_header_t* cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)(port->clb);
	cmdheader += slot;

	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->w = 0;
	cmdheader->a = 1;
	cmdheader->prdtl = 1;
   
	struct achi_hba_cmd_tbl_t* cmdtbl = (achi_hba_cmd_tbl_t*)(uint64_t)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	int i = 0;
	cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)aligned_buf  & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(aligned_buf) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = count * ATAPI_SECTOR_SIZE - 1;	   // 2048 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	ahci_fis_reg_h2d_t* cmdfis = (ahci_fis_reg_h2d_t*)cmdtbl->cfis;
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->featurel = 5;
	cmdfis->command = ATA_CMD_PACKET;

	cmdtbl->acmd[0] = ATAPI_CMD_READ12; // READ(12)
	cmdtbl->acmd[1] = 0;
	cmdtbl->acmd[2] = (uint8_t)((start >> 24)& 0xff);
	cmdtbl->acmd[3] = (uint8_t)((start >> 16)& 0xff);
	cmdtbl->acmd[4] = (uint8_t)((start >> 8)& 0xff);
	cmdtbl->acmd[5] = (uint8_t)((start >> 0)& 0xff);
	cmdtbl->acmd[6] = (count >> 24) & 0xFF;
	cmdtbl->acmd[7] = (count >> 16) & 0xFF;
	cmdtbl->acmd[8] = (count >> 8) & 0xFF;
	cmdtbl->acmd[9] = (count >> 0) & 0xFF;
	cmdtbl->acmd[10] = 0; // group/flags
	cmdtbl->acmd[11] = 0; // control
	cmdtbl->acmd[12] = 0;
	cmdtbl->acmd[13] = 0;
	cmdtbl->acmd[14] = 0;
	cmdtbl->acmd[15] = 0;

	while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};

	if (spin == 1000000) {
		dprintf("Port hung [atapi] (start %lx count %x)\n", start, count);
		return false;
	}

	port->ci = (1 << slot);

	while(true) {
		if ((port->ci & (1<<slot)) == 0) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) {
			dprintf("Read disk error [atapi] (start %lx count %x)\n", start, count);
			return false;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		dprintf("Read disk error [atapi] (start %lx count %x)\n", start, count);
		return false;
	}

	memcpy(buf, aligned_buf, total_bytes);

	add_random_entropy(*buf);

	return true;
}

bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar)
{
	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = ((start & 0xFFFFFFFF00000000) >> 32);
	uint32_t o_count = count;
	port->is = (uint32_t)-1;	   // Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		return false;
	}

	if (!raw_write_buf) {
		raw_write_buf = (uint8_t*)kmalloc_low(HDD_SECTOR_SIZE * 128 + 0x1000);
		aligned_write_buf = (uint8_t*)(((uintptr_t)raw_write_buf + 0xFFF) & ~0xFFF);
	}

	memcpy(aligned_write_buf, buf, o_count * HDD_SECTOR_SIZE);

	ahci_hba_cmd_header_t* cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;

	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t)/sizeof(uint32_t); // Command FIS size
	cmdheader->w = 1;	   // Write device
	cmdheader->prdtl = (uint16_t)((o_count-1)>>4) + 1;	// PRDT entries count

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(ahci_hba_prdt_entry_t));

	uint8_t* wr_buf = aligned_write_buf;

	int i;
	// 8K bytes (16 sectors) per PRDT
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)wr_buf & 0xffffffff);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)wr_buf >> 32) & 0xffffffff);
		cmdtbl->prdt_entry[i].dbc = (16 * HDD_SECTOR_SIZE) - 1;  // 8 KB
		cmdtbl->prdt_entry[i].i = 1;
		wr_buf += 8*1024;    // advance by exactly the size of dbc+1
		count -= 16;      // 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)wr_buf & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)wr_buf >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = (count * HDD_SECTOR_SIZE) - 1;   // 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;  // Command
	cmdfis->command = ATA_CMD_WRITE_DMA_EX;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;  // LBA mode

	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);

	cmdfis->countl = o_count & 0xFF;
	cmdfis->counth = (o_count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [write] (start %lx count %x)\n", start, o_count);
		return false;
	}

	port->ci = (1<<slot); // Issue command

	// Wait for completion
	while (true) {
		if ((port->ci & (1<<slot)) == 0) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Write disk error (start %lx count %x)\n", start, o_count);
			return false;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Write disk error (start %lx count %x)\n", start, o_count);
		return false;
	}

	return true;
}

static void ata_copy_str(char *dst, size_t dst_len, const uint8_t *id_page, int word_off, int word_cnt)
{
	size_t n = (size_t)word_cnt * 2;
	if (n >= dst_len) {
		n = dst_len - 1;
	}

	for (size_t i = 0; i < n; i += 2) {
		dst[i] = (char)id_page[(word_off * 2) + i + 1];
		dst[i + 1] = (char)id_page[(word_off * 2) + i + 0];
	}
	dst[n] = '\0';

	for (long j = n - 1; j >= 0; j--) {
		if (dst[j] == ' ') {
			dst[j] = '\0';
		} else {
			break;
		}
	}
}

static void humanise_capacity(char *out, size_t out_len, uint64_t bytes) {
	const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
	int u = 0;
	uint64_t v = bytes;

	while (v >= 1024 && u < 5) {
		v /= 1024;
		u++;
	}

	snprintf(out, out_len, "%lu %s", v, units[u]);
}

static void build_sata_label(storage_device_t *sd, const uint8_t *id_page) {
	char model[48] = {0};
	char size_str[24] = {0};

	uint16_t w217 = ((const uint16_t *)id_page)[217]; /* 1 = SSD */
	const char *kind = (w217 == 1) ? "SSD" : "HDD";

	ata_copy_str(model, sizeof(model), id_page, 27, 20);
	humanise_capacity(size_str, sizeof(size_str), sd->size * sd->block_size);

	snprintf(sd->ui.label, sizeof(sd->ui.label), "%s - %s (%s)",
		 (model[0] ? model : "ATA Device"),
		 size_str,
		 kind);

	sd->ui.is_optical = false;
}

void init_ahci()
{
	uint8_t* raw = (uint8_t*)kmalloc_low(ATAPI_SECTOR_SIZE * 16);
	uintptr_t raw_addr = (uintptr_t)raw;
	uintptr_t aligned_addr = (raw_addr + (ATAPI_SECTOR_SIZE - 1)) & ~(ATAPI_SECTOR_SIZE - 1);
	aligned_buf = (void*)aligned_addr;

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
	uint32_t irq_num = pci_setup_interrupt("AHCI", ahci_device, logical_cpu_id(), ahci_handler, ahci_base);
	dprintf("AHCI base MMIO: %lx INT %d\n", ahci_base, irq_num);

	probe_port((ahci_hba_mem_t*)ahci_base, ahci_device);
}
