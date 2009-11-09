#include "../include/kernel.h"
#include "../include/iso9660.h"
#include "../include/printf.h"
#include "../include/io.h"
#include "../include/ata.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/filesystem.h"

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

void DumpHex(unsigned char* address, u32int length)
{
	int index = 0;
	for(; index < length; index += 16)
	{
		printf("%04x: ", index);
		int hex = 0;
		for (; hex < 16; ++hex)
			printf("%02x ", address[index + hex]);
		putstring(current_console, " | ");
		for (hex = 0; hex < 16; ++hex)
			put(current_console, (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);

		put(current_console, '\n');
	}
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

	//DumpHex(dirbuffer, 0x150);

	// Iterate each of the entries in this directory, enumerating files
	unsigned char* walkbuffer = dirbuffer;
	int entrycount = 0;
	while (walkbuffer < dirbuffer + lengthbytes)
	{
		entrycount++;
		ISO9660_directory* fentry = (ISO9660_directory*)walkbuffer;

		if (fentry->length == 0)
			break;

		// Skip the first two entries, '.' and '..'
		if (entrycount > 2)
		{
			fentry->filename[fentry->filename_length] = 0;
			printf("File size: %d lba %x\n", fentry->data_length_lsb, fentry->extent_lba_lsb);
			printf("%d/%02d/%02d %02d:%02d:%02d ", fentry->recording_date.years_since_1900 + 1900, fentry->recording_date.month, fentry->recording_date.day, fentry->recording_date.hour, fentry->recording_date.minute, fentry->recording_date.second);
			printf("file_flags: %d ", fentry->file_flags);
			printf("filename: '%s'\n", fentry->filename);
		}
		walkbuffer += fentry->length;
	}

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
