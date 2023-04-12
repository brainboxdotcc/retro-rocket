#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#define FD_MAX 1024
#define IOBUFSZ 8192

#define _O_APPEND   0x00000001
#define _O_CREAT    0x00000002
#define _O_RDONLY   0x00000004
#define _O_WRONLY   0x00000008
#define _O_RDWR     (_O_WRONLY|_O_RDONLY)

/* File flags for fs_directory_entry_t */
#define FS_DIRECTORY	0x00000001	/* Entry is a directory */
#define FS_MOUNTPOINT	0x00000002	/* Entry is a mountpoint */

typedef uint16_t mode_t;

/* Prototypes for filesystem drivers (see filesystem_t) */
typedef void* (*get_directory)(void*);
typedef int (*mount_volume)(const char*, const char*);
typedef bool (*read_file)(void*, uint64_t, uint32_t, unsigned char*);
typedef bool (*write_file)(void*, uint64_t, uint32_t, unsigned char*);
typedef uint64_t (*create_file)(void*, const char*, size_t);
typedef uint64_t (*create_dir)(void*, const char*);
typedef bool (*delete_file)(void*, const char*);
typedef bool (*delete_dir)(void*, const char*);
typedef int (*block_read)(void*, uint64_t, uint32_t, unsigned char*);
typedef int (*block_write)(void*, uint64_t, uint32_t, const unsigned char*);

/* A VFS directory entry. Files AND directories have these,
 * but internally there is also a tree of fs_tree_t* which is used
 * for faster access and caching the structure to RAM
 */
typedef struct fs_directory_entry_t {
	char* filename;		/* Entry name */
	char* alt_filename;	/* Alternative filename (e.g. short filename on fat32) */
	uint16_t year;		/* Creation year */
	uint8_t month;		/* Creation month */
	uint8_t day;		/* Creation day */
	uint8_t hour;		/* Creation hour */
	uint8_t min;		/* Creation minute */
	uint8_t sec;		/* Creation second */
	uint64_t lbapos;	/* On-device position of file (driver specific, e.g. for iso9660
				   it is a raw sector, but for fat32 it is a cluster number) */
	char device_name[16];	/* Device name */				   
	uint32_t device;	/* Device ID (driver specific, for ide devices it is the index) XXX DEPRECATED */
	uint64_t size;		/* File size in bytes */
	uint32_t flags;		/* File flags (FS_*) */
	struct fs_tree_t* directory;	/* Containing directory */
	struct fs_directory_entry_t* next;	/* Next entry */
} fs_directory_entry_t;

/* Defines a filesystem driver. A driver does not need to implement
 * all functions listed here. An unimplemented function should be
 * NULL, e.g. for a readonly filesystem there is no need to implement
 * delete_file or write_file.
 */
typedef struct filesystem_t {
	char name[32];			/* Filesystem name */
	mount_volume mount;		/* mount() entrypoint */
	get_directory getdir;		/* getdir() entrypoint */
	read_file readfile;		/* readfile() entrypoint */
	write_file writefile;		/* writefile() entrypoint */
	create_file createfile;		/* createfile() entrypoint */
	create_dir createdir;		/* createdir() entrypoint */
	delete_file rm;			/* rm() entrypoint */
	delete_dir rmdir;		/* rmdir() entrypoint */
	struct filesystem_t* next;	/* Next entry */
} filesystem_t;

/* Represents a block storage device
 * e.g. a hard disk, DVD-ROM drive, etc.
 */
typedef struct storage_device_t {
	char name[16];		/* Storage device name */
	uint64_t size;		/* Size in bytes */
	block_read blockread;	/* Function pointer for block read routine */
	block_write blockwrite; /* Function pointer for block write routine */
	uint32_t block_size;	/* Size of one block */
	uint64_t opaque1;	/* For device driver use */
	void* opaque2;		/* For device driver use */
	struct storage_device_t* next;
} storage_device_t;

/* Used internally by filesystem.c to cache directories to RAM,
 * and to route requests for that directory through to their
 * driver. Also used to attach a driver initially to its
 * mountpoint unix style. fs_tree_t structs are usually not
 * usable or visible to non-filesystem drivers.
 */
typedef struct fs_tree_t {
	uint8_t dirty;		/* If this is set, the directory needs to be (re-)fetched from the filesystem driver */
	char* name;		/* Directory name */
	struct fs_tree_t* parent;	/* Parent directory name */
	struct fs_tree_t* child_dirs;	/* Linked list of child directories */
	struct fs_directory_entry_t* files;	/* List of files (this also includes directories with FS_DIRECTORY bit set) */
	struct filesystem_t* responsible_driver;	/* The driver responsible for handling this directory */
	uint64_t lbapos;			/* Directory LBA position (driver specific) */
	char device_name[16];	/* Device name */	
	uint64_t device;			/* Directory device ID (driver specific) */
	uint64_t size;			/* Directory size (usually meaningless except to drivers) */
	void* opaque;			/* Opaque data (driver specific data) */
	struct fs_tree_t* next;		/* Next entry for iterating as a linked list (enumerating child directories) */
} fs_tree_t;

typedef enum fs_handle_type_t {
	file_input,
	file_output,
	file_random
} fs_handle_type_t;

/* The data for an open file descriptor.
 * The FD table is an array of pointers to these
 * structs, any closed FD is NULL.
 */
typedef struct fs_handle_t {
	fs_handle_type_t type;		/* Filehandle type */
	unsigned char* inbuf;		/* Input buffer */
	unsigned char* outbuf;		/* Output buffer */
	uint64_t inbufpos;		/* Input buffer position */
	uint64_t outbufpos;		/* Input buffer position */
	uint64_t outbufsize;		/* Output buffer size */
	uint64_t inbufsize;		/* Input buffer size */
	fs_directory_entry_t* file;	/* File which is open */
	uint64_t seekpos;		/* Seek position within file */
	bool cached;			/* Entire file is cached to ram */
} fs_handle_t;


/* Register a new filesystem */
int register_filesystem(filesystem_t* newfs);

/* Find a filesystem by name */
filesystem_t* find_filesystem(const char* name);

/* Register a new storage device */
int register_storage_device(storage_device_t* newdev);

/* Find a storage device by name */
storage_device_t* find_storage_device(const char* name);

/* Read blocks from storage device by name */
int read_storage_device(const char* name, uint64_t start_block, uint32_t bytes, unsigned char* data);

/* Write blocks to storage device by name */
int write_storage_device(const char* name, uint64_t start_block, uint32_t bytes, const unsigned char* data);

/* Attach a filesystem to a vfs directory. The opaque data is optional
 * and if included is driver-specific.
 */
int attach_filesystem(const char* virtual_path, filesystem_t* fs, void* opaque);

/**
 * @brief High level mount function
 * 
 * @param pathname VFS path to mount device/driver to
 * @param device block device name
 * @param filesystem_driver filesystem driver name
 * @return int 1 for success, 0 for failure
 */
int filesystem_mount(const char* pathname, const char* device, const char* filesystem_driver);

/* Initialise the filesystem
 * This loads the DummyFS filesystem which manages the root directory
 * until any other driver is loaded. DummyFS is a dummy and does nothing.
 */
void init_filesystem();

/* Get a list of files in a directory. The directory path must be fully
 * qualified from the root directory and must contain no trailing slash.
 */
fs_directory_entry_t* fs_get_items(const char* pathname);

/**
 * @brief Returns true if the given path is a directory, false if it is a file
 * 
 * @param pathname full qualified vfs path
 * @return true is a directory
 * @return false is a file
 */
bool fs_is_directory(const char* pathname);

/* Retrieve file information on any arbitrary filename.
 */
fs_directory_entry_t* fs_get_file_info(const char* pathandfile);

fs_directory_entry_t* fs_create_file(const char* pathandfile, size_t bytes);

fs_directory_entry_t* fs_create_directory(const char* pathandfile);

/* Read raw bytes from any arbitrary file.
 */
int fs_read_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer);

/* POSIX _open function, opens a file for read or write access,
 * or creates a new file.
 */
int _open(const char *filename, int oflag);

/* POSIX _read function, reads bytes from an open file.
 */
int _read(int fd, void *buffer, unsigned int count);

/* POSIX _write function, writes bytes to an open file.
 */
int _write(int fd, void *buffer, unsigned int count);

/* POSIX _close function, closes an open file.
 */
int _close(uint32_t fd);

/* POSIX _eof function, reports wether or not we have reached
 * the end of file marker on any open file.
 */
int _eof(int fd);

int64_t _lseek(int fd, uint64_t offset, uint64_t origin);

int64_t _tell(int fd);

int unlink(const char *pathname);

int mkdir(const char *pathname, mode_t mode);

int rmdir(const char *pathname);

/**
 * @brief Low level delete file
 * 
 * @param pathandfile path and filename
 * @return true if file was deleted
 */
bool fs_delete_file(const char* pathandfile);

bool fs_delete_directory(const char* pathandfile);

#endif

