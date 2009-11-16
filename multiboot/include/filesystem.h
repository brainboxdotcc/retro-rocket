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

typedef int (*change_directory)(void*, const char*);
typedef int (*read_file)(void*, const char*, u32int, u32int, unsigned char*);
typedef int (*write_file)(void*, const char*, u32int, u32int, unsigned char*);
typedef int (*delete_file)(void*, const char*);

typedef struct FileSystem_t
{
	char name[32];
	change_directory chdir;
	read_file readfile;
	write_file writefile;
	delete_file rm;
	struct FileSystem_t* next;

} FS_FileSystem;

int register_filesystem(FS_FileSystem* newfs);

int attach_filesystem(const char* virtual_path, FS_FileSystem* fs);

#endif
