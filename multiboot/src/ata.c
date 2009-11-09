#include "../include/ata.h"
#include "../include/printf.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/timer.h"
#include "../include/interrupts.h"

ide_device ide_devices[4];
channel channels[2];

unsigned char ide_buf[2048] = {0};
unsigned static char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char ide_read(unsigned char channel, unsigned char reg)
{
	//printf("channel %d base: %x ctrl: %x base+reg: %x\n", channel, channels[channel].base, channels[channel].ctrl, channels[channel].base + reg);
	unsigned char result;
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	if (reg < 0x08) result = inb(channels[channel].base + reg - 0x00);
	else if	(reg < 0x0C) result = inb(channels[channel].base + reg - 0x06);
	else if	(reg < 0x0E) result = inb(channels[channel].ctrl + reg - 0x0A);
	else if	(reg < 0x16) result = inb(channels[channel].bmide + reg - 0x0E);
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	return result;
}

void ide_write(unsigned char channel, unsigned char reg, unsigned char data)
{
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	if (reg < 0x08) outb(channels[channel].base + reg - 0x00, data);
	else if	(reg < 0x0C) outb(channels[channel].base + reg - 0x06, data);
	else if	(reg < 0x0E) outb(channels[channel].ctrl + reg - 0x0A, data);
	else if	(reg < 0x16) outb(channels[channel].bmide + reg - 0x0E, data);
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads)
{
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	asm("pushw %es; movw %ds, %ax; movw %ax, %es");
	if (reg < 0x08) insl(channels[channel].base + reg - 0x00, buffer, quads);
	else if	(reg < 0x0C) insl(channels[channel].base + reg - 0x06, buffer, quads);
	else if	(reg < 0x0E) insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
	else if	(reg < 0x16) insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
	asm("popw %es;");
	if (reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}


unsigned char ide_polling(unsigned char channel, unsigned int advanced_check)
{
	// (I) Delay 400 nanosecond for BSY to be set:
	// -------------------------------------------------
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

	// (II) Wait for BSY to be cleared:
	// -------------------------------------------------
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.

	if (advanced_check)
	{
		unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

		// (III) Check For Errors:
		// -------------------------------------------------
		if (state & ATA_SR_ERR)
			return 2; // Error.

		// (IV) Check If Device fault:
		// -------------------------------------------------
		if (state & ATA_SR_DF )
			return 1; // Device Fault.

		// (V) Check DRQ:
		// -------------------------------------------------
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if (!(state & ATA_SR_DRQ))
			return 3; // DRQ should be set
	}
	return 0; // No Error.

}

unsigned char ide_print_error(unsigned int drive, unsigned char err) {
	
	if (err == 0)
		return err;

	printf("IDE drive %d error:\n ", drive);
	if (err == 1)
	{
		printf("%s", "- Device Fault\n ");
		err = 19;
	}
	else if (err == 2)
	{
		unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
		if (st & ATA_ER_AMNF)
		{
			printf("%s", "- No Address Mark Found\n ");
			err = 7;
		}
		if ((st & ATA_ER_TK0NF) || (st & ATA_ER_MCR) || (st & ATA_ER_MC))
		{
			printf("%s", "- No Media or Media Error\n ");
			err = 3;
		}
		if (st & ATA_ER_ABRT)
		{
			printf("%s", "- Command Aborted\n ");
			err = 20;
		}
		if (st & ATA_ER_IDNF)
		{
			printf("%s", "- ID mark not Found\n ");
			err = 21;
		}
		if (st & ATA_ER_UNC)
		{
			printf("%s", "- Uncorrectable Data Error\n ");
			err = 22;
		}
		if (st & ATA_ER_BBK)
		{
			printf("%s", "- Bad Sectors\n ");
			err = 13;
		}
	} else if (err == 3)
	{
		printf("%s", "- Reads Nothing\n ");
		err = 23;
	}
	else if (err == 4)
	{
		printf("%s", "- Write Protected\n ");
		err = 8;
	}
	printf("- [%s %s] %s\n",
		(const char *[]){"Primary","Secondary"}[ide_devices[drive].channel],
		(const char *[]){"Master", "Slave"}[ide_devices[drive].drive],
		ide_devices[drive].model);

	return err;
}

void ide_initialize()
{
	int type, base, masterslave, k, err, count = 0;
	// 1- Define I/O Ports which interface IDE Controller
	channels[ATA_PRIMARY].base = 0x1F0;
	channels[ATA_PRIMARY].ctrl = 0x3F4;
	channels[ATA_SECONDARY].base = 0x170;
	channels[ATA_SECONDARY].ctrl = 0x374;
	channels[ATA_PRIMARY].bmide = 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = 8; // Bus Master IDE

	// 2- Disable IRQs temporarily
	ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);


	// 3- Detect ATA-ATAPI Devices
	// Refactored 9th Nov 2009. Did not properly detect ATAPI devices,
	// due to not testing for ATAPI IDENTIFY ABORT soon enough before
	// trying to wait on the command.
	for (base = 0x1F0; base >= 0x170; base -= 0x80)
	{
		for (masterslave = 0xA0; masterslave <= 0xB0; masterslave += 0x10)
		{
			err = 0;
			// Send ATA IDENTIFY
			outb(base + 6, masterslave);
			outb(base + 7, 0xEC);
			unsigned char x = inb(base + 7);
			//printf("%s %s Status: %d\n", base == 0x1F0 ? "Primary" : "Secondary", masterslave == 0xA0 ? "master" : "slave", x);

			// Nothing at all on this connection
			if (x == 0)
				continue;

			// Check status of IDENTIFY command first
			unsigned char y = inb(base + 4);
			unsigned char z = inb(base + 5);
			type = IDE_ATA;
			if (y == 0x14 && z == 0xEB)
			{
				// ATA IDENTIFY command aborted, this is SATA or ATAPI
				outb(base + 6, masterslave);
				outb(base + 7, 0xA1);
				type = IDE_ATAPI;
			}
			while (1)
			{
				x = inb(base + 7);
				// Wait for BSY (busy) bit to clear
				if ((x & 0x80) == 0)
				{
					// If BSY is clear, check if DRQ is set
					if ((x & 0x08) != 0)
						break;
				}
				// Check after all else if ERROR is set
				else if ((x & 1) != 0)
					break;
			}
	
			if (err == 0)
			{
				// We found a drive! Add it to our list of found devices.
				ide_read_buffer(base == 0x1F0 ? 0 : 1, ATA_REG_DATA, (unsigned int)ide_buf, 128);

				ide_devices[count].reserved = 1;
				ide_devices[count].type = type;
				ide_devices[count].channel = (base == 0x1F0 ? 0 : 1);
				ide_devices[count].drive = (masterslave == 0xA0 ? 0 : 1);
				ide_devices[count].sign = ((unsigned short *) (ide_buf + ATA_IDENT_DEVICETYPE))[0];
				ide_devices[count].capabilities = ((unsigned short *) (ide_buf + ATA_IDENT_CAPABILITIES))[0];
				ide_devices[count].commandsets = ((unsigned int *) (ide_buf + ATA_IDENT_COMMANDSETS))[0];

				if (ide_devices[count].commandsets & (1<<26))
				{
					ide_devices[count].size	= ((unsigned int *) (ide_buf + ATA_IDENT_MAX_LBA_EXT))[0];
				}
				else
				{
					ide_devices[count].size	= ((unsigned int *) (ide_buf + ATA_IDENT_MAX_LBA))[0];
				}

				for(k = ATA_IDENT_MODEL; k < (ATA_IDENT_MODEL+40); k+=2)
				{
					ide_devices[count].model[k - ATA_IDENT_MODEL] = ide_buf[k + 1];
					ide_devices[count].model[(k + 1) - ATA_IDENT_MODEL] = ide_buf[k];
				}
				// Backpeddle over the name removing trailing spaces
				k = 39;
				while (ide_devices[count].model[k] == ' ')
					ide_devices[count].model[k--] = 0;

				if (type == IDE_ATAPI)
					printf("%s: ATAPI CDROM\n", ide_devices[count].model);
				else
					printf("%s: %dGb IDE\n", ide_devices[count].model, ide_devices[count].size / 1024 / 1024 / 2);
				count++;
			}

		}
	}
	register_interrupt_handler(IRQ14, ide_irq);
	register_interrupt_handler(IRQ15, ide_irq);
}

unsigned char ide_ata_access(unsigned char direction, unsigned char drive, unsigned int lba, unsigned char numsects, unsigned int edi)
{
	unsigned char lba_mode, dma, cmd;
	unsigned char lba_io[6];
	unsigned int channel	= ide_devices[drive].channel; // Read the Channel.
	unsigned int slavebit = ide_devices[drive].drive; // Read the Drive [Master/Slave]
	unsigned int bus = channels[channel].base; // The Bus Base, like [0x1F0] which is also data port.
	unsigned int words = 256; // Approx. all ATA-Drives have a sector-size of 512-byte.
	unsigned short cyl, i; unsigned char head, sect, err;

	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);
	// (I) Select one from LBA28, LBA48 or CHS;
	if (lba >= 0x10000000)
	{
		// LBA48:
		lba_mode = 2;
		lba_io[0] = (lba & 0x000000FF)>> 0;
		lba_io[1] = (lba & 0x0000FF00)>> 8;
		lba_io[2] = (lba & 0x00FF0000)>>16;
		lba_io[3] = (lba & 0xFF000000)>>24;
		lba_io[4] = 0; // We said that we lba is integer, so 32-bit are enough to access 2TB.
		lba_io[5] = 0; // We said that we lba is integer, so 32-bit are enough to access 2TB.
		head = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} 
	else if (ide_devices[drive].capabilities & 0x200)
	{
		// LBA28:
		lba_mode = 1;
		lba_io[0] = (lba & 0x00000FF)>> 0;
		lba_io[1] = (lba & 0x000FF00)>> 8;
		lba_io[2] = (lba & 0x0FF0000)>>16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0; // These Registers are not used here.
		lba_io[5] = 0; // These Registers are not used here.
		head = (lba & 0xF000000)>>24;
	}
	else
	{
		// CHS:
		lba_mode = 0;
		sect = (lba % 63) + 1;
		cyl = (lba + 1	- sect)/(16*63);
		lba_io[0] = sect;
		lba_io[1] = (cyl>>0) & 0xFF;
		lba_io[2] = (cyl>>8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect)%(16*63)/(63); // Head number is written to HDDEVSEL lower 4-bits.
	}
	// (II) See if Drive Supports DMA or not;
	dma = 0; // Supports or doesn't, we don't support !!!
	// (III) Wait if the drive is busy;
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait if Busy.
	// (IV) Select Drive from the controller;
	if (lba_mode == 0)
		ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit<<4) | head);	// Select Drive CHS.
	else
		ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit<<4) | head);	// Select Drive LBA.
	// (V) Write Parameters;
	if (lba_mode == 2)
	{
		ide_write(channel, ATA_REG_SECCOUNT1, 0);
		ide_write(channel, ATA_REG_LBA3, lba_io[3]);
		ide_write(channel, ATA_REG_LBA4, lba_io[4]);
		ide_write(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ide_write(channel, ATA_REG_SECCOUNT0, numsects);
	ide_write(channel, ATA_REG_LBA0, lba_io[0]);
	ide_write(channel, ATA_REG_LBA1, lba_io[1]);
	ide_write(channel, ATA_REG_LBA2, lba_io[2]);
	if (lba_mode == 0 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;	
	if (lba_mode == 2 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO_EXT;	
	if (lba_mode == 0 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA_EXT;
	if (lba_mode == 0 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO_EXT;
	if (lba_mode == 0 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA_EXT;
	ide_write(channel, ATA_REG_COMMAND, cmd);				// Send the Command.
	if (dma)
		if (direction == 0);
		// DMA Read.
		else; // DMA Write.
	else if (direction == 0)
	// PIO Read.
	for (i = 0; i < numsects; i++)
	{
		if ((err = ide_polling(channel, 1)))
			return err; // Polling, then set error and exit if there is.
		asm("pushw %es");
		asm("rep insw"::"c"(words), "d"(bus), "D"(edi)); // Receive Data.
		asm("popw %es");
		edi += (words*2);
	}
	else
	{
		// PIO Write.
		for (i = 0; i < numsects; i++)
		{
			ide_polling(channel, 0); // Polling.
			asm("pushw %ds");
			asm("rep outsw"::"c"(words), "d"(bus), "S"(edi)); // Send Data
			asm("popw %ds");
			edi += (words*2);
		}
		ide_write(channel, ATA_REG_COMMAND, (char []) {	ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT }[lba_mode]);
		ide_polling(channel, 0); // Polling.
	}

	return 0; // Easy, ... Isn't it?
}

void ide_wait_irq()
{
	while (!ide_irq_invoked);
	ide_irq_invoked = 0;
}

void ide_irq(registers_t regs)
{
	ide_irq_invoked = 1;
}

unsigned char ide_atapi_read(unsigned char drive, unsigned int lba, unsigned char numsects, unsigned int edi)
{
	unsigned int channel = ide_devices[drive].channel;
	unsigned int slavebit = ide_devices[drive].drive;
	unsigned int bus = channels[channel].base;
	unsigned int words = 2048 / 2; // Sector Size in Words, Almost All ATAPI Drives has a sector size of 2048 bytes.
	unsigned char	err; int i;
	// Enable IRQs:
	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);
	// (I): Setup SCSI Packet:
	// ------------------------------------------------------------------
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
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_HDDEVSEL, slavebit<<4);
	// (III): Delay 400 nanosecond for select to complete:
	// ------------------------------------------------------------------
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	// (IV): Inform the Controller that we use PIO mode:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_FEATURES, 0);		 // PIO mode.
	// (V): Tell the Controller the size of buffer:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);	// Lower Byte of Sector Size.
	ide_write(channel, ATA_REG_LBA2, (words * 2)>>8);	// Upper Byte of Sector Size.
	// (VI): Send the Packet Command:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);		// Send the Command.
	// (VII): Waiting for the driver to finish or invoke an error:
	// ------------------------------------------------------------------
	if ((err = ide_polling(channel, 1)))
		return err;		 // Polling and return if error.
	// (VIII): Sending the packet data:
	// ------------------------------------------------------------------
	asm("rep	outsw"::"c"(6), "d"(bus), "S"(atapi_packet));	// Send Packet Data
	// (IX): Recieving Data:
	// ------------------------------------------------------------------
	for (i = 0; i < numsects; i++)
	{
		ide_wait_irq();					// Wait for an IRQ.
		if ((err = ide_polling(channel, 1)))
		 return err;		// Polling and return if error.
		asm("pushw %es");
		asm("rep insw"::"c"(words), "d"(bus), "D"(edi));// Receive Data.
		asm("popw %es");
		edi += (words*2);
	}
	// (X): Waiting for an IRQ:
	// ------------------------------------------------------------------
	ide_wait_irq();

	// (XI): Waiting for BSY & DRQ to clear:
	// ------------------------------------------------------------------
	while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));
	return 0; // Easy, ... Isn't it?
}

int ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, unsigned int edi)
{
	int i;
	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	}
	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
	{
		printf("Seek to invalid position %d\n", lba+numsects);					 // Seeking to invalid position.
		return 0;
	}
	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else
	{
		unsigned char err;
		if (ide_devices[drive].type == IDE_ATA)
		{
			err = ide_ata_access(ATA_READ, drive, lba, numsects, edi);
		}
		else if (ide_devices[drive].type == IDE_ATAPI)
		{
			for (i = 0; i < numsects; i++)
			{
				err = ide_atapi_read(drive, lba + i, 1, edi + (i*2048));
			}
		}
		ide_print_error(drive, err);
		return !err;
	}
	return 0;
}

int ide_write_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, unsigned int edi)
{
	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	}
	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
	{
		printf("Seek to invalid position %d\n", lba+numsects);					 // Seeking to invalid position.
		return 0;
	}
	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else
	{
		unsigned char err;
		if (ide_devices[drive].type == IDE_ATA)
			err = ide_ata_access(ATA_WRITE, drive, lba, numsects, edi);
		else if (ide_devices[drive].type == IDE_ATAPI)
			err = 4; // Write-Protected.
		ide_print_error(drive, err);

		return !err;
	}
	return 0;
}

int ide_atapi_eject(unsigned char drive)
{
	unsigned int channel = ide_devices[drive].channel;
	unsigned int slavebit = ide_devices[drive].drive;
	unsigned int bus = channels[channel].base;
	unsigned char err = 0;
	ide_irq_invoked = 0;

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);
		return 0;
	}
	// 2: Check if drive isn't ATAPI:
	// ==================================
	else if (ide_devices[drive].type == IDE_ATA)
	{
		printf("Command aborted on drive %d\n", drive);		 // Command Aborted.
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
		ide_print_error(drive, err);
		return !err;
	}
	return 0;
}

