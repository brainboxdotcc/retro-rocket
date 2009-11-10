#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "kernel.h"

#define FS_DIRECTORY 0x00000001

typedef struct FS_DirectoryEntryTag
{
	char* filename;
	u16int year;
	u8int month;
	u8int day;
	u8int hour;
	u8int min;
	u8int sec;
	u32int lbapos;
	u32int device;
	u32int size;
	u32int flags;
	struct FS_DirectoryEntryTag* next;

} FS_DirectoryEntry;

#endif
