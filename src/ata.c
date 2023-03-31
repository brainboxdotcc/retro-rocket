#include <kernel.h>
#include <interrupt.h>
#include <ata.h>

ide_device ide_devices[4];
channel channels[2];

uint8_t ide_buf[2048] = {0};
unsigned static volatile char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t ide_read(uint8_t channel, uint8_t reg)
{
	uint8_t result;
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}
	if (reg < 0x08) {
		result = inb(channels[channel].base + reg - 0x00);
	} else if (reg < 0x0C) {
		result = inb(channels[channel].base + reg - 0x06);
	} else if (reg < 0x0E) {
		result = inb(channels[channel].ctrl + reg - 0x0A);
	} else if (reg < 0x16) {
		result = inb(channels[channel].bmide + reg - 0x0E);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}
	return result;
}

void ide_write(uint8_t channel, uint8_t reg, uint8_t data)
{
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}
	if (reg < 0x08) {
		outb(channels[channel].base + reg - 0x00, data);
	} else if (reg < 0x0C) {
		outb(channels[channel].base + reg - 0x06, data);
	} else if (reg < 0x0E) {
		outb(channels[channel].ctrl + reg - 0x0A, data);
	} else if (reg < 0x16) {
		outb(channels[channel].bmide + reg - 0x0E, data);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}
}

void ide_read_buffer(uint8_t channel, uint8_t reg, uint64_t buffer, uint32_t quads)
{
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}
	if (reg < 0x08) {
		insl(channels[channel].base + reg - 0x00, buffer, quads);
	} else if (reg < 0x0C) {
		insl(channels[channel].base + reg - 0x06, buffer, quads);
	} else if (reg < 0x0E) {
		insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
	} else if (reg < 0x16) {
		insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}
}


uint8_t ide_polling(uint8_t channel, uint32_t advanced_check)
{
	// (I) Delay 400 nanosecond for BSY to be set:
	// -------------------------------------------------
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);

	// (II) Wait for BSY to be cleared
	// -------------------------------------------------
	uint64_t timer_start = get_ticks();
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY && (get_ticks() - timer_start) < 100 && (get_ticks() >= timer_start)); // Wait for BSY to be zero.

	if (get_ticks() - timer_start > 100) {
		return ATA_POLL_TIMEOUT;	/* Interrupt didnt arrive within 2 secs */
	}

	if (advanced_check) {
		uint8_t state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

		// (III) Check For Errors:
		// -------------------------------------------------
		if (state & ATA_SR_ERR) {
			return ATA_POLL_GENRAL_ERROR; // Error.
		}

		// (IV) Check If Device fault:
		// -------------------------------------------------
		if (state & ATA_SR_DF) {
			return ATA_POLL_DEVICE_FAULT; // Device Fault.
		}

		// (V) Check DRQ:
		// -------------------------------------------------
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if (!(state & ATA_SR_DRQ)) {
			return ATA_POLL_DRQ_NOT_SET; // DRQ should be set
		}
	}
	return ATA_POLL_NO_ERROR; // No Error.

}

uint8_t ide_print_error(uint32_t drive, uint8_t err)
{
	kprintf("IDE drive %d error: ", drive);
	if (err == 1) {
		kprintf("Device Fault");
		err = 19;
	} else if (err == 2) {
		uint8_t st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
		if (st & ATA_ER_AMNF) {
			kprintf("No Address Mark Found");
			err = 7;
		}
		if ((st & ATA_ER_TK0NF) || (st & ATA_ER_MCR) || (st & ATA_ER_MC)) {
			kprintf("No Media or Media Error");
			err = 3;
		}
		if (st & ATA_ER_ABRT) {
			kprintf("Command Aborted");
			err = 20;
		}
		if (st & ATA_ER_IDNF) {
			kprintf("ID mark not Found");
			err = 21;
		}
		if (st & ATA_ER_UNC) {
			kprintf("Uncorrectable Data Error");
			err = 22;
		}
		if (st & ATA_ER_BBK) {
			kprintf("Bad Sectors");
			err = 13;
		}
	}
	else if (err == 3) {
		kprintf("Reads Nothing");
		err = 23;
	}
	else if (err == 4) {
		kprintf("Write Protected");
		err = 8;
	}
	kprintf(" - [%s %s] %s\n",
		(const char *[]){"Primary","Secondary"}[ide_devices[drive].channel],
		(const char *[]){"master", "slave"}[ide_devices[drive].drive],
		ide_devices[drive].model);

	return err;
}

/**
 * @brief Handler function for block reads from ATA devices from read_storage_device()
 * 
 * @param dev block storage device
 * @param start starting block
 * @param bytes number of bytes to read (this will always be rounded up in increments of block size)
 * @param buffer buffer to receive blocks
 * @return int 1 on successful read, 0 on failure
 */
int storage_device_ide_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}
	uint32_t divided_length = bytes / sd->block_size;
	if (divided_length == 0) {
		divided_length = 1;
	}
	return ide_read_sectors((uint8_t)sd->opaque1, divided_length, start, (uint64_t)buffer);
}

/**
 * @brief Handler function for block writes to ATA devices from write_storage_device()
 * 
 * @param dev block storage device
 * @param start starting block
 * @param bytes number of bytes to read (this will always be rounded up in increments of block size)
 * @param buffer buffer to receive blocks
 * @return int 1 on successful read, 0 on failure
 */
int storage_device_ide_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}
	uint32_t divided_length = bytes / sd->block_size;
	if (divided_length == 0) {
		divided_length = 1;
	}
	return ide_write_sectors((uint8_t)sd->opaque1, divided_length, start, (uint64_t)buffer);
}

void ide_initialise(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4)
{
	int type, base, masterslave, k, err, count = 0;
	// 1- Define I/O Ports which interface IDE Controller
	channels[ATA_PRIMARY].base = BAR0;
	channels[ATA_PRIMARY].ctrl = BAR1;
	channels[ATA_SECONDARY].base = BAR2;
	channels[ATA_SECONDARY].ctrl = BAR3;
	channels[ATA_PRIMARY].bmide = 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = 8; // Bus Master IDE

	// 2- Disable IRQs temporarily
	ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	// 3- Detect ATA-ATAPI Devices
	// Refactored 9th Nov 2009. Did not properly detect ATAPI devices,
	// due to not testing for ATAPI IDENTIFY ABORT soon enough before
	// trying to wait on the command.
	for (base = BAR0; base >= BAR2; base -= 0x80) {
		for (masterslave = 0xA0; masterslave <= 0xB0; masterslave += 0x10) {
			err = 0;
			// Send ATA IDENTIFY
			outb(base + 6, masterslave);
			outb(base + 7, 0xEC);
			uint8_t x = inb(base + 7);

			// Nothing at all on this connection
			if (x == 0) {
				continue;
			}

			// Check status of IDENTIFY command first
			uint8_t y = inb(base + 4);
			uint8_t z = inb(base + 5);
			type = IDE_ATA;
			if (y == 0x14 && z == 0xEB) {
				// ATA IDENTIFY command aborted, this is SATA or ATAPI
				outb(base + 6, masterslave);
				outb(base + 7, 0xA1);
				type = IDE_ATAPI;
			}
			uint64_t timer_start = get_ticks();
			while (1) {
				x = inb(base + 7);
				// Wait for BSY (busy) bit to clear
				if ((x & 0x80) == 0) {
					// If BSY is clear, check if DRQ is set
					if ((x & 0x08) != 0) {
						break;
					}
				} else if ((x & 1) != 0) {
					// Check after all else if ERROR is set
					break;
				}

				if (get_ticks() - timer_start > 100) {
					break;	// Timeout! - Added March 2012
				}
			}
	
			if (err == 0) {
				// We found a drive! Add it to our list of found devices.
				ide_read_buffer(base == 0x1F0 ? 0 : 1, ATA_REG_DATA, (uint64_t)ide_buf, 128);

				//dump_hex(ide_buf, 128);

				ide_devices[count].reserved = 1;
				ide_devices[count].type = type;
				ide_devices[count].channel = (base == 0x1F0 ? 0 : 1);
				ide_devices[count].drive = (masterslave == 0xA0 ? 0 : 1);
				ide_devices[count].sign = ((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE))[0];
				ide_devices[count].capabilities = ((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES))[0];
				ide_devices[count].commandsets = ((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS))[0];

				if (ide_devices[count].commandsets & (1<<26)) {
					ide_devices[count].size	= ((uint64_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT)) [0] & 0xFFFFFFFFFFFull;
				} else {
					ide_devices[count].size	= ((uint64_t*)(ide_buf + ATA_IDENT_MAX_LBA))[0] & (uint64_t)0xFFFFFFFFFFFFull; /* 48 bits */
				}

				for (k = ATA_IDENT_MODEL; k < (ATA_IDENT_MODEL + 40); k += 2) {
					ide_devices[count].model[k - ATA_IDENT_MODEL] = ide_buf[k + 1];
					ide_devices[count].model[(k + 1) - ATA_IDENT_MODEL] = ide_buf[k];
				}
				// Backpeddle over the name removing trailing spaces
				k = 39;
				while (ide_devices[count].model[k] == ' ') {
					ide_devices[count].model[k--] = 0;
				}

				storage_device_t* sd = (storage_device_t*)kmalloc(sizeof(storage_device_t));
				if (type == IDE_ATAPI) {
					make_unique_device_name("cd", sd->name);
					sd->block_size = 2048;
				} else {
					make_unique_device_name("hd", sd->name);
					sd->block_size = 512;
				}
				sd->blockread = storage_device_ide_block_read;
				sd->blockwrite = storage_device_ide_block_write;
				sd->opaque1 = count;
				sd->opaque2 = NULL;
				register_storage_device(sd);
				count++;
			}

		}
	}
	register_interrupt_handler(IRQ14, ide_irq);
	register_interrupt_handler(IRQ15, ide_irq);
}

uint8_t ide_ata_access(uint8_t direction, uint8_t drive, uint64_t lba, uint16_t numsects, uint64_t buffer_address)
{
	uint8_t lba_mode, dma, cmd;
	uint8_t lba_io[6];
	uint32_t channel	= ide_devices[drive].channel; // Read the Channel.
	uint32_t slavebit = ide_devices[drive].drive; // Read the Drive [Master/Slave]
	uint32_t bus = channels[channel].base; // The Bus Base, like [0x1F0] which is also data port.
	uint32_t words = 256; // Approx. all ATA-Drives have a sector-size of 512-byte.
	uint16_t cyl, i; uint8_t head, sect, err;

	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);
	// (I) Select one from LBA28, LBA48 or CHS;
	if (lba >= 0x10000000) {
		// LBA48:
		lba_mode = 2;
		lba_io[0] = (lba & 0x0000000000FFull)>>0;
		lba_io[1] = (lba & 0x00000000FF00ull)>>8;
		lba_io[2] = (lba & 0x000000FF0000ull)>>16;
		lba_io[3] = (lba & 0x0000FF000000ull)>>24;
		lba_io[4] = (lba & 0x00FF00000000ull)>>32;
		lba_io[5] = (lba & 0xFF0000000000ull)>>40;
		head = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} 
	else if (ide_devices[drive].capabilities & 0x200) {
		// LBA28:
		lba_mode = 1;
		lba_io[0] = (lba & 0x0000FF)>>0;
		lba_io[1] = (lba & 0x00FF00)>>8;
		lba_io[2] = (lba & 0xFF0000)>>16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba & 0xF000000)>>24;
	} else {
		// CHS:
		lba_mode = 0;
		sect = (lba % 63) + 1;
		cyl = (lba + 1	- sect) / (16*63);
		lba_io[0] = sect;
		lba_io[1] = (cyl>>0) & 0xFF;
		lba_io[2] = (cyl>>8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
	}
	// (II) See if Drive Supports DMA or not;
	dma = 0; // Supports or doesn't, we don't support !!!
	// (III) Wait if the drive is busy;
	uint64_t timer_start = get_ticks();
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY && get_ticks() - timer_start < 100); // Wait if Busy.
	// (IV) Select Drive from the controller;
	if (lba_mode == 0) {
		ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head);	// Select Drive CHS.
	} else {
		ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);	// Select Drive LBA.
	}
	// (V) Write Parameters;
	if (lba_mode == 2) {
		ide_write(channel, ATA_REG_SECCOUNT1, ((numsects >> 8) & 0xFF));
		ide_write(channel, ATA_REG_LBA3, lba_io[3]);
		ide_write(channel, ATA_REG_LBA4, lba_io[4]);
		ide_write(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ide_write(channel, ATA_REG_SECCOUNT0, numsects & 0xFF);
	ide_write(channel, ATA_REG_LBA0, lba_io[0]);
	ide_write(channel, ATA_REG_LBA1, lba_io[1]);
	ide_write(channel, ATA_REG_LBA2, lba_io[2]);
	if (lba_mode == 0 && dma == 0 && direction == 0) {
		cmd = ATA_CMD_READ_PIO;
	}
	if (lba_mode == 1 && dma == 0 && direction == 0) {
		cmd = ATA_CMD_READ_PIO;	
	}
	if (lba_mode == 2 && dma == 0 && direction == 0) {
		cmd = ATA_CMD_READ_PIO_EXT;	
	}
	if (lba_mode == 0 && dma == 1 && direction == 0) {
		cmd = ATA_CMD_READ_DMA;
	}
	if (lba_mode == 1 && dma == 1 && direction == 0) {
		cmd = ATA_CMD_READ_DMA;
	}
	if (lba_mode == 2 && dma == 1 && direction == 0) {
		cmd = ATA_CMD_READ_DMA_EXT;
	}
	if (lba_mode == 0 && dma == 0 && direction == 1) {
		cmd = ATA_CMD_WRITE_PIO;
	}
	if (lba_mode == 1 && dma == 0 && direction == 1) {
		cmd = ATA_CMD_WRITE_PIO;
	}
	if (lba_mode == 2 && dma == 0 && direction == 1) {
		cmd = ATA_CMD_WRITE_PIO_EXT;
	}
	if (lba_mode == 0 && dma == 1 && direction == 1) {
		cmd = ATA_CMD_WRITE_DMA;
	}
	if (lba_mode == 1 && dma == 1 && direction == 1) {
		cmd = ATA_CMD_WRITE_DMA;
	}
	if (lba_mode == 2 && dma == 1 && direction == 1) {
		cmd = ATA_CMD_WRITE_DMA_EXT;
	}
	ide_write(channel, ATA_REG_COMMAND, cmd);				// Send the Command.
	if (dma) {
		if (direction == 0) {
			// DMA Read.
		} else {
			// DMA Write.
		}
	} else if (direction == 0) {
		// PIO Read.
		for (i = 0; i < numsects; i++) {
			if ((err = ide_polling(channel, 1))) {
				return err; // Polling, then set error and exit if there is.
			}
			insw(bus, buffer_address, words);
			buffer_address += (words*2);
		}
	} else {
		// PIO Write.
		for (i = 0; i < numsects; i++) {
			if ((err = ide_polling(channel, 0))) { // Polling.
				return err;
			}
			outsw(bus, buffer_address, words);
			buffer_address += (words*2);
		}
		ide_write(channel, ATA_REG_COMMAND, (char []) {	ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT }[lba_mode]);
		if ((err = ide_polling(channel, 0))) { // Polling.
			return err;
		}
	}

	return 0; // Easy, ... Isn't it?
}

void ide_wait_irq()
{
	while (!ide_irq_invoked);
	ide_irq_invoked = 0;
}

void ide_irq(uint8_t isr, uint64_t error, uint64_t irq)
{
	ide_irq_invoked = 1;
}

uint8_t ide_atapi_read(uint8_t drive, uint64_t lba, uint8_t numsects, uint64_t buffer_address)
{
	uint32_t channel = ide_devices[drive].channel;
	uint32_t slavebit = ide_devices[drive].drive;
	uint32_t bus = channels[channel].base;
	uint32_t words = 2048 / 2; // Sector Size in Words, Almost All ATAPI Drives has a sector size of 2048 bytes.
	uint8_t	err; int i;
	// Enable IRQs:
	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);
	// (I): Setup SCSI Packet:
	atapi_packet[ 0] = ATAPI_CMD_READ;
	atapi_packet[ 1] = 0x0;
	atapi_packet[ 2] = (lba>>24) & 0xFF;
	atapi_packet[ 3] = (lba>>16) & 0xFF;
	atapi_packet[ 4] = (lba>> 8) & 0xFF;
	atapi_packet[ 5] = (lba>> 0) & 0xFF;
	atapi_packet[ 6] = 0x0;
	atapi_packet[ 7] = 0x0;
	atapi_packet[ 8] = 0x0;
	atapi_packet[ 9] = numsects;
	atapi_packet[10] = 0x0;
	atapi_packet[11] = 0x0;
	// (II): Select the Drive:
	ide_write(channel, ATA_REG_HDDEVSEL, slavebit<<4);
	// (III): Delay 400 nanosecond for select to complete:
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	// (IV): Inform the Controller that we use PIO mode:
	ide_write(channel, ATA_REG_FEATURES, 0);		 // PIO mode.
	// (V): Tell the Controller the size of buffer:
	ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);	// Lower Byte of Sector Size.
	ide_write(channel, ATA_REG_LBA2, (words * 2)>>8);	// Upper Byte of Sector Size.
	// (VI): Send the Packet Command:
	ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);		// Send the Command.
	// (VII): Waiting for the driver to finish or invoke an error:
	if ((err = ide_polling(channel, 1))) {
		return err;		// Polling and return if error.
	}
	// (VIII): Sending the packet data:
	outsw(bus, &atapi_packet, 6);
	// (IX): Recieving Data:
	for (i = 0; i < numsects; i++) {
		ide_wait_irq();				// Wait for an IRQ.
		if ((err = ide_polling(channel, 1))) {
			return err;		// Polling and return if error.
		}
		insw(bus, buffer_address, words);
		buffer_address += (words*2);
	}
	// (X): Waiting for an IRQ:
	ide_wait_irq();
	// (XI): Waiting for BSY & DRQ to clear:
	uint64_t timer_start = get_ticks();
	while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ) && get_ticks() - timer_start < 100);
	return 0; // Easy, ... Isn't it?
}

int ide_read_sectors(uint8_t drive, uint16_t numsects, uint64_t lba, uint64_t buffer_address)
{
	int i;
	if (drive > 3 || ide_devices[drive].reserved == 0) {
		// 1: Check if the drive present
		kprintf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	} else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA)) {
		// 2: Check if inputs are valid:
		kprintf("Seek to invalid position LBA=0x%08x\n", lba+numsects);					 // Seeking to invalid position.
		return 0;
	} else {
		// 3: Read in PIO Mode through Polling & IRQs:
		uint8_t err;
		if (ide_devices[drive].type == IDE_ATA) {
			err = ide_ata_access(ATA_READ, drive, lba, numsects, buffer_address);
		} else if (ide_devices[drive].type == IDE_ATAPI) {
			for (i = 0; i < numsects; i++) {
				err = ide_atapi_read(drive, lba + i, 1, buffer_address + (i*2048));
			}
		}
		if (err) {
			ide_print_error(drive, err);
		}
		return !err;
	}
	return 0;
}

int ide_write_sectors(uint8_t drive, uint16_t numsects, uint64_t lba, uint64_t buffer_address)
{
	if (drive > 3 || ide_devices[drive].reserved == 0) {
		// 1: Check if the drive is present
		kprintf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	} else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA)) {
		// 2: Check if inputs are valid
		kprintf("Seek to invalid position %d\n", lba+numsects);					 // Seeking to invalid position.
		return 0;
	} else {
		// 3: Write in PIO Mode through Polling & IRQs:
		uint8_t err;
		if (ide_devices[drive].type == IDE_ATA) {
			err = ide_ata_access(ATA_WRITE, drive, lba, numsects, buffer_address);
		} else {
			if (ide_devices[drive].type == IDE_ATAPI) {
				err = 4; // Write-Protected.
			}
		}
		if (err) {
			ide_print_error(drive, err);
		}

		return !err;
	}
	return 0;
}

int ide_atapi_eject(uint8_t drive)
{
	uint32_t channel = ide_devices[drive].channel;
	uint32_t slavebit = ide_devices[drive].drive;
	uint32_t bus = channels[channel].base;
	uint8_t err = 0;
	ide_irq_invoked = 0;

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		kprintf("Drive %d not found\n", drive);
		return 0;
	}
	// 2: Check if drive isn't ATAPI:
	// ==================================
	else if (ide_devices[drive].type == IDE_ATA)
	{
		kprintf("Command aborted on drive %d\n", drive);		 // Command Aborted.
		return 0;
	}
	// 3: Eject ATAPI Driver:
	// ============================================
	else
	{
		// Enable IRQs:
		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

		// (I): Setup SCSI Packet:
		// ------------------------------------------------------------------
		atapi_packet[ 0] = ATAPI_CMD_EJECT;
		atapi_packet[ 1] = 0x00;
		atapi_packet[ 2] = 0x00;
		atapi_packet[ 3] = 0x00;
		atapi_packet[ 4] = 0x02;
		atapi_packet[ 5] = 0x00;
		atapi_packet[ 6] = 0x00;
		atapi_packet[ 7] = 0x00;
		atapi_packet[ 8] = 0x00;
		atapi_packet[ 9] = 0x00;
		atapi_packet[10] = 0x00;
		atapi_packet[11] = 0x00;

		// (II): Select the Drive:
		// ------------------------------------------------------------------
		ide_write(channel, ATA_REG_HDDEVSEL, slavebit<<4);

		// (III): Delay 400 nanosecond for select to complete:
		// ------------------------------------------------------------------
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

		// (IV): Send the Packet Command:
		// ------------------------------------------------------------------
		ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);		// Send the Command.

		// (V): Waiting for the driver to finish or invoke an error:
		// ------------------------------------------------------------------
		if (!(err = ide_polling(channel, 1)))
		{
			asm("rep outsw"::"c"(6), "d"(bus), "S"(atapi_packet));// Send Packet Data
			ide_wait_irq();					// Wait for an IRQ.
			err = ide_polling(channel, 1);			// Polling and get error code.
			if (err == 3)
				err = 0; // DRQ is not needed here.
		}
		if (err)
			ide_print_error(drive, err);
		return !err;
	}
	return 0;
}

