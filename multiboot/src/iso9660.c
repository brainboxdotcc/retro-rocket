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

FS_DirectoryEntry* ParseDirectory(iso9660* info, u32int start_lba, u32int lengthbytes);


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
	info->volume_name = (char*)kmalloc(strlen(pvd->volumeidentifier) + 1);
	for (ptr = pvd->volumeidentifier; *ptr; ++ptr)
		info->volume_name[j++] = *ptr;
	// Null-terminate volume name
	info->volume_name[j] = 0;

	info->pathtable_lba = pvd->lsb_pathtable_L_lba;
	info->rootextent_lba = pvd->root_directory.extent_lba_lsb;
	info->root = ParseDirectory(info, pvd->root_directory.extent_lba_lsb, pvd->root_directory.data_length_lsb);

	return info->root != 0;
}

FS_DirectoryEntry* ParseDirectory(iso9660* info, u32int start_lba, u32int lengthbytes)
{
	unsigned char* dirbuffer = (unsigned char*)kmalloc(lengthbytes);
	int j;

	_memset(dirbuffer, 0, lengthbytes);

	if (!ide_read_sectors(info->drivenumber, lengthbytes / 2048, start_lba, (unsigned int)dirbuffer))
	{
		printf("ISO9660: Could not read LBA sectors 0x%x+0x%x when loading directory!\n", start_lba, lengthbytes / 2048);
		kfree(dirbuffer);
		return NULL;
	}

	FS_DirectoryEntry* first = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
	FS_DirectoryEntry* thisentry = first;
	first->next = 0;

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
			thisentry->filename = (char*)kmalloc(fentry->filename_length + 1);
			j = 0;
			char* ptr = fentry->filename;
			// Stop at end of string or at ; which seperates the version id from the filename.
			// We don't want the version ids.
			for (; j < fentry->filename_length && *ptr != ';'; ++ptr)
				thisentry->filename[j++] = tolower(*ptr);
			thisentry->filename[j] = 0;

			thisentry->year = fentry->recording_date.years_since_1900 + 1900;
			thisentry->month = fentry->recording_date.month;
			thisentry->day = fentry->recording_date.day;
			thisentry->hour = fentry->recording_date.hour;
			thisentry->min = fentry->recording_date.minute;
			thisentry->sec = fentry->recording_date.second;
			thisentry->device = info->drivenumber;
			thisentry->lbapos = fentry->extent_lba_lsb;
			thisentry->size = fentry->data_length_lsb;
			thisentry->flags = 0;
			if (fentry->file_flags & 0x02)
				thisentry->flags |= FS_DIRECTORY;

			FS_DirectoryEntry* next = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
			thisentry->next = next;
			next->next = 0;
			thisentry = next;
		}
		walkbuffer += fentry->length;
	}

	kfree(dirbuffer);
	return first;
}

void ParseSVD(iso9660* info, unsigned char* buffer)
{
}

void ParseVPD(iso9660* info, unsigned char* buffer)
{
}

int iso_change_directory(iso9660* info, const char* newdirectory)
{
	FS_DirectoryEntry* currentdir = info->root;
	for(; currentdir->next; currentdir = currentdir->next)
	{
		if ((currentdir->flags & FS_DIRECTORY) && !strcmp(newdirectory, currentdir->filename))
		{
			FS_DirectoryEntry* newdir = ParseDirectory(info, currentdir->lbapos, currentdir->size);
			if (newdir)
			{
				FREE_LINKED_LIST(FS_DirectoryEntry*, info->root);
				info->root = newdir;
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

iso9660* iso_mount_volume(u32int drivenumber)
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