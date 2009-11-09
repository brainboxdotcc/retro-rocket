#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "kernel.h"

typedef struct
{
	char* filename;
	u16int year;
	u8int month;
	u8int day;
	u8int hour;
	u8int min;
	u8int sec;
	u8int flags;
	u32int lbapos;
	u32int device;
	struct FS_DirectoryEntry* next;

} FS_DirectoryEntry;

#endif
