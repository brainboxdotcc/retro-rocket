#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "kernel.h"

#define FD_MAX 128
#define IOBUFSZ 4096

#define _O_APPEND   0x00000001
#define _O_CREAT    0x00000002
#define _O_RDONLY   0x00000004
#define _O_WRONLY   0x00000008
#define _O_RDWR     (_O_WRONLY|_O_RDONLY)

/* File flags for FS_DirectoryEntry */
#define FS_DIRECTORY	0x00000001	/* Entry is a directory */
#define FS_MOUNTPOINT	0x00000002	/* Entry is a mountpoint */

/* Prototypes for filesystem drivers (see FS_FileSystem) */
typedef void* (*get_directory)(void*);
typedef int (*read_file)(void*, u32int, u32int, unsigned char*);
typedef int (*write_file)(void*, u32int, u32int, unsigned char*);
typedef int (*delete_file)(void*);

struct FS_Tree_t;

/* A VFS directory entry. Files AND directories have these,
 * but internally there is also a tree of FS_Tree* which is used
 * for faster access and caching the structure to RAM
 */
typedef struct FS_DirectoryEntryTag
{
	char* filename;		/* Entry name */
	u16int year;		/* Creation year */
	u8int month;		/* Creation month */
	u8int day;		/* Creation day */
	u8int hour;		/* Creation hour */
	u8int min;		/* Creation minute */
	u8int sec;		/* Creation second */
	u32int lbapos;		/* LBA position of file (driver specific) */
	u32int device;		/* Device ID (driver specific) */
	u32int size;		/* File size in bytes */
	u32int flags;		/* File flags (FS_*) */
	struct FS_Tree_t* directory;	/* Containing directory */
	struct FS_DirectoryEntryTag* next;	/* Next entry */

} FS_DirectoryEntry;

/* Defines a filesystem driver. A driver does not need to implement
 * all functions listed here. An unimplemented function should be
 * NULL, e.g. for a readonly filesystem there is no need to implement
 * delete_file or write_file.
 */
typedef struct FileSystem_t
{
	char name[32];		/* Filesystem name */
	get_directory getdir;	/* getdir() entrypoint */
	read_file readfile;	/* readfile() entrypoint */
	write_file writefile;	/* writefile() entrypoint */
	delete_file rm;		/* rm() entrypoint */
	struct FileSystem_t* next;	/* Next entry */
} FS_FileSystem;

/* Used internally by filesystem.c to cache directories to RAM,
 * and to route requests for that directory through to their
 * driver. Also used to attach a driver initially to its
 * mountpoint unix style. FS_Tree structs are usually not
 * usable or visible to non-filesystem drivers.
 */
typedef struct FS_Tree_t
{
	u8int dirty;		/* If this is set, the directory needs to be (re-)fetched from the filesystem driver */
	char* name;		/* Directory name */
	struct FS_Tree_t* parent;	/* Parent directory name */
	struct FS_Tree_t* child_dirs;	/* Linked list of child directories */
	struct FS_DirectoryEntry* files;	/* List of files (this also includes directories with FS_DIRECTORY bit set) */
	struct FS_FileSystem* responsible_driver;	/* The driver responsible for handling this directory */
	u32int lbapos;			/* Directory LBA position (driver specific) */
	u32int device;			/* Directory device ID (driver specific) */
	u32int size;			/* Directory size (usually meaningless except to drivers) */
	void* opaque;			/* Opaque data (driver specific data) */
	struct FS_Tree_t* next;		/* Next entry for iterating as a linked list (enumerating child directories) */
} FS_Tree;

typedef enum
{
	file_input,
	file_output,
	file_random
} FS_HandleType;

/* The data for an open file descriptor.
 * The FD table is an array of pointers to these
 * structs, any closed FD is NULL.
 */
typedef struct FS_Handle_t
{
	FS_HandleType type;		/* Filehandle type */
	unsigned char inbuf[IOBUFSZ];	/* Input buffer size */
	unsigned char outbuf[IOBUFSZ];	/* Output buffer size */
	FS_DirectoryEntry* file;	/* File which is open */
	u32int seekpos;			/* Seek position within file */
} FS_Handle;


/* Register a new filesystem */
int register_filesystem(FS_FileSystem* newfs);

/* Attach a filesystem to a vfs directory. The opaque data is optional
 * and if included is driver-specific.
 */
int attach_filesystem(const char* virtual_path, FS_FileSystem* fs, void* opaque);

/* Initialise the filesystem
 * This loads the DummyFS filesystem which manages the root directory
 * until any other driver is loaded. DummyFS is a dummy and does nothing.
 */
void init_filesystem();

/* Get a list of files in a directory. The directory path must be fully
 * qualified from the root directory and must contain no trailing slash.
 */
FS_DirectoryEntry* fs_get_items(const char* pathname);

/* Retrieve file information on any arbitrary filename.
 */
FS_DirectoryEntry* fs_get_file_info(const char* pathandfile);

/* Read raw bytes from any arbitrary file.
 */
int fs_read_file(FS_DirectoryEntry* file, u32int start, u32int length, unsigned char* buffer);

/* POSIX _open function, opens a file for read or write access,
 * or creates a new file.
 */
int _open(const char *filename, int oflag);

/* POSIX _read function, reads bytes from an open file.
 */
int _read(int fd, void *buffer, unsigned int count);

/* POSIX _close function, closes an open file.
 */
int _close(u32int fd);

/* POSIX _eof function, reports wether or not we have reached
 * the end of file marker on any open file.
 */
int _eof(int fd);

long _lseek(int fd, long offset, int origin);

long _tell(int fd);

#endif

