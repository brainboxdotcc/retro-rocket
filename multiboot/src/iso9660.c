#include "../include/kernel.h"
#include "../include/iso9660.h"
#include "../include/printf.h"
#include "../include/io.h"
#include "../include/ata.h"

void ParseBOOT(iso9660* info, unsigned char* buffer)
{
}

void ParsePVD(iso9660* info, unsigned char* buffer)
{
	PVD* pvd = (PVD*)buffer;
}

void ParseSVD(iso9660* info, unsigned char* buffer)
{
}

void ParseVPD(iso9660* info, unsigned char* buffer)
{
}

iso9660* mount_volume(u32int drivenumber)
{
	unsigned char* buffer = (unsigned char*)kmalloc(2048);
	iso9660* info = (iso9660*)kmalloc(sizeof(iso9660));
	_memset(buffer, 0, 2048);
	int VolumeDescriptorPos = PVD_LBA;
	while (1)
	{
		ide_read_sectors(drivenumber, 1, VolumeDescriptorPos++, (unsigned int)buffer);
		unsigned char VolumeDescriptorID = buffer[0];
		if (VolumeDescriptorID == 0xFF)
		{
			// Volume descriptor terminator
			break;
		}
		else if (VolumeDescriptorID == 0x00)
		{
			ParseBOOT(info, buffer);
		}
		else if (VolumeDescriptorID == 0x01)
		{
			// Primary volume descriptor
			ParsePVD(info, buffer);
		}
		else if (VolumeDescriptorID == 0x02)
		{
			// Supplementary volume descriptor
			ParseSVD(info, buffer);
		}
		else if (VolumeDescriptorID == 0x03)
		{
			// Volume partition descriptor
			ParseVPD(info, buffer);
		}
		else if (VolumeDescriptorID >= 0x04 && VolumeDescriptorID <= 0xFE)
		{
			// Reserved and unknown ID
			printf("ISO9660: Unknown volume descriptor 0x%x!\n", VolumeDescriptorID);
		}
	}

	return info;
}
