#include <kernel.h>

/* Tested:
 * 8086:2922 - 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]
 */

void* aligned_buf = NULL;

bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar);
bool ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar);
uint64_t ahci_read_size(ahci_hba_port_t *port, ahci_hba_mem_t* abar);

void ahci_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void* opaque)
{
	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)opaque;
	dprintf("AHCI IRQ\n");
	uint32_t is = abar->is;
	for (int i = 0; i < 32; i++) {
		if (is & (1u << i)) {
			uint32_t pis = abar->ports[i].is;
			abar->ports[i].is = pis; // ack by writing back
			dprintf("AHCI IRQ: port %d, PxIS=%08x\n", i, pis);
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

// Start command engine
void start_cmd(ahci_hba_port_t *port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);
	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}
 
// Stop command engine
void stop_cmd(ahci_hba_port_t *port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;
	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;
 
	// Wait until FR (bit14), CR (bit15) are cleared
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
	stop_cmd(port);	// Stop command engine
 
	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = (kmalloc_low(0x100000) & 0xFFFFFFFFFFFFF000) + 0x1000;
	port->clbu = 0;
	memset((void*)((uint64_t)port->clb), 0, 1024);

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = port->clb + (32 * 1024) + (portno << 8);
	port->fbu = 0;
	memset((void*)((uint64_t)port->fb), 0, 256);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)((uint64_t)port->clb);
	for (int i = 0; i < 32; i++) {
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
					// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = port->clb + (40 * 1024) + (portno << 13) + (i << 8);
		cmdheader[i].ctbau = 0;
		memset((void*)(uint64_t)cmdheader[i].ctba, 0, 256);
	}
	start_cmd(port);	// Start command engine
}

int storage_device_ahci_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}

	// Convert byte length to sectors, rounding up
	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	if (check_type(port) == AHCI_DEV_SATAPI) {
		// ATAPI device, 32 KiB bounce buffer → 16 sectors max per command
		const uint32_t max_per_cmd = 16;
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
		// Hard disk, 64 KiB bounce buffer → 128 sectors max per command
		const uint32_t max_per_cmd = 128;
		while (sectors > 0) {
			uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
			if (!ahci_read(port, start, this_xfer, (uint16_t*)buffer, abar)) {
				return false;
			}
			start   += this_xfer;
			buffer  += this_xfer * sd->block_size;
			sectors -= this_xfer;
		}
		return true;
	}
}

int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}

	// Convert byte length to sectors, rounding up
	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	// Write bounce buffer can only handle 16 sectors (8 KiB) at a time
	const uint32_t max_per_cmd = 128;

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


void probe_port(ahci_hba_mem_t *abar, pci_dev_t dev)
{
	// Search disk in implemented ports
	uint32_t pi = abar->pi;
	int i = 0;
	while (i < 32) {
		// iterate all 32 bits of the active ports field
		if (pi & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				kprintf(
					"AHCI@%04x:%04x: %s storage, Port #%d\n",
					pci_read(dev, PCI_VENDOR_ID), pci_read(dev, PCI_DEVICE_ID),
					dt == AHCI_DEV_SATA ? "SATA" : "ATAPI", i
				);
				port_rebase(&abar->ports[i], i);

				// Enable interrupts for this port
				abar->ports[i].ie = 0xFFFFFFFF;
				abar->ghc |= (1 << 1);  // global IE


				storage_device_t* sd = NULL;
				sd = kmalloc(sizeof(storage_device_t));
				if (!sd) {
					return;
				}
				sd->opaque1 = i;
				sd->opaque2 = (void*)abar;
				sd->blockread = storage_device_ahci_block_read;
				sd->blockwrite = storage_device_ahci_block_write;
				sd->size = dt == AHCI_DEV_SATA ? ahci_read_size(&abar->ports[i], abar) : SIZE_MAX;
				make_unique_device_name(dt == AHCI_DEV_SATA ? "hd" : "cd", sd->name, sizeof(sd->name));
				sd->block_size = dt == AHCI_DEV_SATA ? HDD_SECTOR_SIZE : ATAPI_SECTOR_SIZE;
				register_storage_device(sd);
			}
		}
 
		pi >>= 1;
		i ++;
	}
}

// Find a free command list slot
int find_cmdslot(ahci_hba_port_t *port, ahci_hba_mem_t *abar)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	int cmdslots = (abar->cap & 0x0f00) >> 8;
	for (int i=0; i<cmdslots; i++) {
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	dprintf("Cannot find free command list entry\n");
	return -1;
}
 
bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar)
{
	static uint8_t *raw_read_buf = NULL;
	static uint8_t *aligned_read_buf = NULL;

	// Allocate a single reusable DMA-safe bounce buffer if not done already
	// (big enough for 128 sectors = 64KiB at once, adjust if you need larger)
	if (!raw_read_buf) {
		raw_read_buf = (uint8_t*)kmalloc_low(HDD_SECTOR_SIZE * 128 + 0x1000);
		aligned_read_buf = (uint8_t*)(((uintptr_t)raw_read_buf + 0xFFF) & ~0xFFF);
	}

	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = (uint32_t)((start >> 32) & 0xFFFFFFFF);

	// Clear pending interrupt bits
	port->is = (uint32_t)-1;

	int spin = 0;
	int slot = find_cmdslot(port, abar);
	if (slot == -1) {
		return false;
	}

	// Command header for this slot
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size
	cmdheader->w   = 0;   // read
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1; // PRDT entries count (ceil(count/16))

	// Command table
	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	// DMA pointer for PRDT setup (use aligned bounce buffer, not caller buf)
	uint8_t *rd_buf = aligned_read_buf;

	// Fill PRDT entries: each PRDT entry can cover 16 sectors (8KiB)
	int i = 0;
	for (i = 0; i < cmdheader->prdtl - 1; i++) {
		cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)rd_buf & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)rd_buf) >> 32);
		cmdtbl->prdt_entry[i].dbc  = 8 * 1024 - 1; // 16 sectors (8KiB)
		cmdtbl->prdt_entry[i].i    = 1;            // interrupt on completion
		rd_buf += 8 * 1024;
		count  -= 16;
	}
	// Last PRDT entry for the remainder
	cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)rd_buf & 0xFFFFFFFF);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)rd_buf) >> 32);
	cmdtbl->prdt_entry[i].dbc  = (count * HDD_SECTOR_SIZE) - 1; // exact size
	cmdtbl->prdt_entry[i].i    = 1;

	// Build command FIS
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

	cmdfis->countl  = count & 0xFF;
	cmdfis->counth  = (count >> 8) & 0xFF;

	// Wait until port is ready
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [read] (start %lx count %x)\n", start, count);
		return false;
	}

	// Issue command
	port->ci = 1 << slot;

	// Wait for completion
	while (true) {
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Read disk error (start %lx count %x)\n", start, count);
			return false;
		}
	}

	// Final error check
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Read disk error (start %lx count %x)\n", start, count);
		return false;
	}

	// Copy out of bounce buffer into caller’s buffer
	memcpy(buf, aligned_read_buf, (count * HDD_SECTOR_SIZE));

	add_random_entropy(*buf);

	return true;
}


uint8_t* raw_buf = NULL;
uint8_t* buf = NULL;
static uint8_t* raw_write_buf = NULL;
static uint8_t* aligned_write_buf = NULL;

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

	memcpy(aligned_write_buf, buf, count * HDD_SECTOR_SIZE);

	ahci_hba_cmd_header_t* cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;

	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t)/sizeof(uint32_t); // Command FIS size
	cmdheader->w = 1;	   // Write device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(ahci_hba_prdt_entry_t));

	uint8_t* wr_buf = aligned_write_buf;

	int i;
	// 8K bytes (16 sectors) per PRDT
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)wr_buf & 0xffffffff);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)wr_buf >> 32) & 0xffffffff);
		cmdtbl->prdt_entry[i].dbc = 8*1024 - 1;  // 8 KB
		cmdtbl->prdt_entry[i].i = 1;
		wr_buf += 8*1024;    // advance by exactly the size of dbc+1
		count -= 16;      // 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba  = (uint32_t)((uint64_t)wr_buf & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)wr_buf >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = (count<<9)-1;   // 512 bytes per sector
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

	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port hung [write] (start %lx count %x)\n", start, count);
		return false;
	}

	port->ci = (1<<slot); // Issue command

	// Wait for completion
	while (true) {
		if ((port->ci & (1<<slot)) == 0) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Write disk error (start %lx count %x)\n", start, count);
			return false;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Write disk error (start %lx count %x)\n", start, count);
		return false;
	}

	return true;
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
