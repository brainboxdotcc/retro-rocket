#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "kernel.h"

#define FS_DIRECTORY 0x00000001

//struct FS_FileSystem;

typedef void* (*get_directory)(void*);
typedef int (*read_file)(void*, const char*, u32int, u32int, unsigned char*);
typedef int (*write_file)(void*, const char*, u32int, u32int, unsigned char*);
typedef int (*delete_file)(void*, const char*);

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

typedef struct FileSystem_t
{
	char name[32];
	get_directory getdir;
	read_file readfile;
	write_file writefile;
	delete_file rm;
	struct FileSystem_t* next;
} FS_FileSystem;

typedef struct FS_Tree_t
{
	u8int dirty;
	char* name;
	struct FS_Tree_t* parent;
	u32int num_child_dirs;
	struct FS_Tree_t** child_dirs;
	struct FS_DirectoryEntry* files;
	struct FS_FileSystem* responsible_driver;
	u32int lbapos;
	u32int device;
	u32int size;
	void* opaque;
} FS_Tree;


int register_filesystem(FS_FileSystem* newfs);

int attach_filesystem(const char* virtual_path, FS_FileSystem* fs);

void init_filesystem();

#endif
