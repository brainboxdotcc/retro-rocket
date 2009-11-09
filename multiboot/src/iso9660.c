#include "../include/kernel.h"
#include "../include/iso9660.h"
#include "../include/printf.h"
#include "../include/io.h"
#include "../include/ata.h"
#include "../include/string.h"
#include "../include/kmalloc.h"

#define VERIFY_ISO9660(n) (n->standardidentifier[0] == 'C' && n->standardidentifier[1] == 'D' \
				&& n->standardidentifier[2] == '0' && n->standardidentifier[3] == '0' && n->standardidentifier[4] == '1')

int ParseDirectory(iso9660* info, u32int start_lba, u32int lengthbytes);


void ParseBOOT(iso9660* info, unsigned char* buffer)
{
}

int ParsePVD(iso9660* info, unsigned char* buffer)
{
	PVD* pvd = (PVD*)buffer;
	if (!VERIFY_ISO9660(pvd))
	{
		putstring(current_console, "ISO9660: Invalid PVD found, identifier is not 'CD001'\n");
		return 0;
	}
	char* ptr = pvd->volumeidentifier + 31;
	for (; ptr != pvd->volumeidentifier && *ptr == ' '; --ptr)
	{
		// Backpeddle over the trailing spaces in the volume name
		if (*ptr == ' ')
			*ptr = 0;
	}

	int j = 0;
	info->volume_name = (char*)kmalloc(strlen(pvd->volumeidentifier));
	for (ptr = pvd->volumeidentifier; *ptr; ++ptr)
		info->volume_name[j++] = *ptr;
	// Null-terminate volume name
	info->volume_name[j] = 0;

	info->pathtable_lba = pvd->lsb_pathtable_L_lba;
	info->rootextent_lba = pvd->root_directory.extent_lba_lsb;

	pvd->root_directory.filename[pvd->root_directory.filename_length] = 0;

	return ParseDirectory(info, pvd->root_directory.extent_lba_lsb, pvd->root_directory.data_length_lsb);
}

int ParseDirectory(iso9660* info, u32int start_lba, u32int lengthbytes)
{
	unsigned char* dirbuffer = (unsigned char*)kmalloc(lengthbytes);
	int j;

	_memset(dirbuffer, 0, lengthbytes);

	if (!ide_read_sectors(info->drivenumber, lengthbytes / 2048, start_lba, (unsigned int)dirbuffer))
	{
		printf("ISO9660: Could not read LBA sectors 0x%x+0x%x when loading directory!\n", start_lba, lengthbytes / 2048);
		return 0;
	}

	for(j = 0; j < lengthbytes; ++j)
		put(current_console, dirbuffer[j]);

	kfree(dirbuffer);

	return 1;
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
	u32int VolumeDescriptorPos = PVD_LBA;
	info->drivenumber = drivenumber;
	while (1)
	{
		if (!ide_read_sectors(drivenumber, 1, VolumeDescriptorPos++, (unsigned int)buffer))
		{
			printf("ISO9660: Could not read LBA sector 0x%x!\n", VolumeDescriptorPos);
			kfree(info);
			return NULL;
		}
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
			if (!ParsePVD(info, buffer))
			{
				kfree(info);
				return NULL;
			}
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
			printf("ISO9660: WARNING: Unknown volume descriptor 0x%x at LBA 0x%x!\n", VolumeDescriptorID, VolumeDescriptorPos);
		}
	}

	return info;
}
