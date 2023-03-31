#include <kernel.h>

/* 8086:2922 - 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode] */

void ahci_handler(uint8_t isr, uint64_t error, uint64_t irq)
{
	dprintf("AHCI interrupt 18\n");
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
 
	switch (port->sig)
	{
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
	for (int i=0; i<32; i++)
	{
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
	uint32_t divided_length = bytes / sd->block_size;
	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];
	if (divided_length == 0) {
		divided_length = 1;
	}
	return check_type(port) == AHCI_DEV_SATAPI ? ahci_atapi_read(port, start, divided_length, (uint16_t*)buffer, abar) : ahci_read(port, start, divided_length, (uint16_t*)buffer, abar);
}

int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer)
{

	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}
	uint32_t divided_length = bytes / sd->block_size;
	ahci_hba_mem_t* abar = (ahci_hba_mem_t*)sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];
	if (divided_length == 0) {
		divided_length = 1;
	}
	return ahci_write(port, start, divided_length, (char*)buffer, abar);
}

int hddcount = 0, cdromcount = 0;

void probe_port(ahci_hba_mem_t *abar)
{
	// Search disk in implemented ports
	uint32_t pi = abar->pi;
	int i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			int dt = check_type(&abar->ports[i]);
			storage_device_t* sd = NULL;
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				sd = (storage_device_t*)kmalloc(sizeof(storage_device_t));
				sd->opaque1 = i;
				sd->opaque2 = (void*)abar;
				sd->blockread = storage_device_ahci_block_read;
				sd->blockwrite = storage_device_ahci_block_write;
			}
			if (dt == AHCI_DEV_SATA && sd) {
				dprintf("SATA drive found at port %d\n", i);
				port_rebase(&abar->ports[i], i);
				sprintf(sd->name, "hd%d", hddcount++);
				sd->block_size = 512;
				register_storage_device(sd);
				dprintf("Registered\n");
			} else if (dt == AHCI_DEV_SATAPI && sd) {
				dprintf("SATAPI drive found at port %d\n", i);
				port_rebase(&abar->ports[i], i);
				sprintf(sd->name, "cd%d", cdromcount++);
				sd->block_size = 2048;
				register_storage_device(sd);
				dprintf("Registered\n");
			} else if (dt == AHCI_DEV_SEMB) {
				dprintf("SEMB drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_PM) {
				dprintf("PM drive found at port %d\n", i);
			} else {
				dprintf("No drive found at port %d\n", i);
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
	for (int i=0; i<cmdslots; i++)
	{
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	dprintf("Cannot find free command list entry\n");
	return -1;
}
 
bool ahci_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar)
{
	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = ((start & 0xFFFFFFFF00000000) >> 32);
	port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port, abar);
	if (slot == -1)
		return false;
 
	ahci_hba_cmd_header_t *cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count
 
	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(ahci_hba_prdt_entry_t));
 
	// 8K bytes (16 sectors) per PRDT
	int i = 0;
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
		cmdtbl->prdt_entry[i].dbc = 8*1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = (count<<9)-1;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
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
		dprintf("Port is hung\n");
		return false;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	// Wait for completion
	while (true) {
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1<<slot)) == 0) 
			break;
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Read disk error\n");
			return false;
		}
	}
 
	// Check again
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Read disk error\n");
		return false;
	}
 
	return true;
}

uint8_t ahci_atapi_read(ahci_hba_port_t *port, uint64_t start, uint32_t count, uint16_t *buf, ahci_hba_mem_t* abar)
{
	port->is = (uint32_t)-1;
	int spin = 0;
	int slot = (int)find_cmdslot(port, abar);
	if (slot == -1) {
		printf("No free command slots\n");
		return 0;
	}

	ahci_hba_cmd_header_t* cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)(port->clb);
	cmdheader += slot;

	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
	cmdheader->w = 0;
	cmdheader->a = 1;
	cmdheader->prdtl = 1;
   
	struct achi_hba_cmd_tbl_t* cmdtbl = (achi_hba_cmd_tbl_t*)(uint64_t)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl - 1) * sizeof(ahci_hba_prdt_entry_t));

	// 8K bytes (16 sectors) per PRDT
	int i = 0;
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
		cmdtbl->prdt_entry[i].dbc = 4 * 2048-1;   // 8K bytes as 2048 byte sectors
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;  // 4K words
		count -= 4;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = count * 2048 - 1;	   // 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	ahci_fis_reg_h2d_t* cmdfis = (ahci_fis_reg_h2d_t*)cmdtbl->cfis;
	memset(cmdfis, 0, sizeof(ahci_fis_reg_h2d_t));

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->featurel = 5;
	cmdfis->command = ATA_CMD_PACKET;

	cmdtbl->acmd[0] = ATAPI_CMD_READ;
	cmdtbl->acmd[1] = 0;
	cmdtbl->acmd[2] = (uint8_t)((start >> 24)& 0xff);
	cmdtbl->acmd[3] = (uint8_t)((start >> 16)& 0xff);
	cmdtbl->acmd[4] = (uint8_t)((start >> 8)& 0xff);
	cmdtbl->acmd[5] = (uint8_t)((start >> 0)& 0xff);
	cmdtbl->acmd[6] = 0;
	cmdtbl->acmd[7] = 0;
	cmdtbl->acmd[8] = 0;
	cmdtbl->acmd[9] = (uint8_t)(count & 0xff);
	cmdtbl->acmd[10] = 0;
	cmdtbl->acmd[11] = 0;
	cmdtbl->acmd[12] = 0;
	cmdtbl->acmd[13] = 0;
	cmdtbl->acmd[14] = 0;
	cmdtbl->acmd[15] = 0;

	while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		printf("Port is hung\n");
		return 0;
	}

	port->ci = (1 << slot);

	while(1) {
		if ((port->ci & (1<<slot)) == 0) break;
		if (port->is & HBA_PxIS_TFES) {
			return 0;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		return 0;
	}

	return 1;
}

bool ahci_write(ahci_hba_port_t *port, uint64_t start, uint32_t count, char *buf, ahci_hba_mem_t* abar)
{
	uint32_t startl = (start & 0xFFFFFFFF);
	uint32_t starth = ((start & 0xFFFFFFFF00000000) >> 32);
	port->is = (uint32_t)-1;	   // Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port, abar);

	ahci_hba_cmd_header_t* cmdheader = (ahci_hba_cmd_header_t*)(uint64_t)port->clb;
	cmdheader += slot;

	cmdheader->cfl = sizeof(ahci_fis_reg_h2d_t)/sizeof(uint32_t); // Command FIS size
	cmdheader->w = 1;	   // Write device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count

	achi_hba_cmd_tbl_t *cmdtbl = (achi_hba_cmd_tbl_t*)((uint64_t)cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(achi_hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(ahci_hba_prdt_entry_t));

	int i;
	// 8K bytes (16 sectors) per PRDT
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
		cmdtbl->prdt_entry[i].dbc = 8*1024-1; // 8K bytes
		//cmdtbl->prdt_entry[i].i = 1;
		buf += 8*1024;  // 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t)((uint64_t)buf & 0xffffffff);
	cmdtbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(buf) >> 32) & 0xffffffff);
	cmdtbl->prdt_entry[i].dbc = (count<<9)-1;   // 512 bytes per sector

	// Setup command
	ahci_fis_reg_h2d_t *cmdfis = (ahci_fis_reg_h2d_t*)(&cmdtbl->cfis);

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

	cmdfis->countl = (count & 0xff);
	cmdfis->counth = (count >> 8);

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		dprintf("Port is hung\n");
		return false;
	}

	port->ci = (1<<slot); // Issue command

	// Wait for completion
	while (true) {
		if ((port->ci & (1<<slot)) == 0)
		break;
		if (port->is & HBA_PxIS_TFES) { // Task file error
			dprintf("Write disk error\n");
			return false;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES) {
		dprintf("Write disk error\n");
		return false;
	}

	return true;
}


void init_ahci()
{
	dprintf("*** INIT AHCI ***\n");
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
	uint32_t ahci_status = pci_read(ahci_device, PCI_STATUS);

	dprintf("AHCI base MMIO: %08x status: %08x\n", ahci_base, ahci_status);

	/* Check for MSI capability */
	if (!(ahci_status & 0x10)) {
		dprintf("No MSI support on AHCI device\n");
		return;
	}

	uint32_t capabilities_ptr = pci_read(ahci_device, PCI_CAPABILITIES);
	dprintf("Capabilities ptr=%02x\n", capabilities_ptr);

	uint32_t current = capabilities_ptr, config_space;
	while ((config_space = pci_read(ahci_device, current))) {
		uint8_t id = config_space & 0xFF;
		uint32_t next_capability = (config_space & 0xFF00) >> 8;
		dprintf("id=%02x next=%02x v=%08x\n", id, next_capability, config_space);
		if (id == 0x05) {
			/* MSI capability */
			uint32_t vector = 20 + 32;
			uint32_t new_message_data = (vector & 0xFF);
			uint32_t new_message_address = get_local_apic();
			pci_write(ahci_device, current + 0x04, new_message_address);
			pci_write(ahci_device, current + 0x08, 0);
			pci_write(ahci_device, current + 0x0C, new_message_data);

			register_interrupt_handler(vector, ahci_handler);
		}
		current = next_capability;
		if (next_capability == 0) {
			break;
		}
	}

	probe_port((ahci_hba_mem_t*)ahci_base);
}
