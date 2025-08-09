/**
 * @file filesystem.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

/**
 * @brief Maximum number of file descriptors which can be open at the same time
 */
#define FD_MAX 1024

/**
 * @brief Default size of IO buffer in open file
 */
#define IOBUFSZ 8192

/* Used by _open() */
#define _O_APPEND	0x00000001
#define _O_CREAT	0x00000002
#define _O_RDONLY	0x00000004
#define _O_WRONLY	0x00000008
#define _O_RDWR		(_O_WRONLY|_O_RDONLY)

/* File flags for fs_directory_entry_t */
#define FS_DIRECTORY	0x00000001	/* Entry is a directory */
#define FS_MOUNTPOINT	0x00000002	/* Entry is a mountpoint */

/* Declared for use in mkdir, ignored by the OS at present */
typedef uint16_t mode_t;

/* Prototypes for filesystem drivers (see filesystem_t) */
typedef void* (*get_directory)(void*);
typedef int (*mount_volume)(const char*, const char*);
typedef bool (*read_file)(void*, uint64_t, uint32_t, unsigned char*);
typedef bool (*write_file)(void*, uint64_t, uint32_t, unsigned char*);
typedef uint64_t (*create_file)(void*, const char*, size_t);
typedef bool (*truncate_file)(void*, size_t);
typedef uint64_t (*create_dir)(void*, const char*);
typedef bool (*delete_file)(void*, const char*);
typedef bool (*delete_dir)(void*, const char*);
typedef uint64_t (*get_free_space)(void*);

/* Prototypes for block storage device drivers (see storage_device_t) */
typedef int (*block_read)(void*, uint64_t, uint32_t, unsigned char*);
typedef int (*block_write)(void*, uint64_t, uint32_t, const unsigned char*);

typedef enum fs_error_t {
	FS_ERR_NO_ERROR = 0,
	FS_ERR_UNSUPPORTED,
	FS_ERR_NO_MORE_FDS,
	FS_ERR_INVALID_FD,
	FS_ERR_INVALID_ARG,
	FS_ERR_NO_SUCH_DIRECTORY,
	FS_ERR_INVALID_FILEPATH,
	FS_ERR_FILE_EXISTS,
	FS_ERR_OUT_OF_MEMORY,
	FS_ERR_IO,
	FS_ERR_SEEK_PAST_END,
	FS_ERR_NOT_OPEN_FOR_OUTPUT,
	FS_ERR_NOT_OPEN_FOR_INPUT,
	FS_ERR_TRUNCATE_BEYOND_LENGTH,
	FS_ERR_NO_SUCH_FILE,
	FS_ERR_INVALID_FILENAME,
	FS_ERR_CYCLE_DETECTED,
	FS_ERR_BROKEN_DIRECTORY,
	FS_ERR_INVALID_GEOMETRY,
	FS_ERR_NO_SPACE,
	FS_ERR_OUTSIDE_VOLUME,
	FS_ERR_NOT_RFS,
	FS_ERR_NOT_A_FILE,
	FS_ERR_VFS_DATA,
	FS_ERR_NOT_A_DIRECTORY,
	FS_ERR_DIRECTORY_NOT_EMPTY,
	FS_ERR_FILE_HAS_VANISHED,
	FS_ERR_DIRECTORY_EXISTS,
	FS_ERR_INVALID_PVD,
	FS_ERR_INVALID_SVD,
	FS_ERR_OVERSIZED_DIRECTORY,
	FS_ERR_BUFFER_WOULD_OVERFLOW,
	FS_ERR_NO_SUCH_DEVICE,
	FS_ERR_LFN_TOO_LONG,
	FS_ERR_IS_EXFAT,
	FS_ERR_BAD_CLUSTER,
} fs_error_t;

/**
 * @brief A VFS directory entry. Files AND directories have these,
 * but internally there is also a tree of fs_tree_t* which is used
 * for faster access and caching the structure to RAM.
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

/**
 * @brief Defines a filesystem driver.
 * 
 * A driver does not need to implement all functions listed here.
 * An unimplemented function should be NULL.
 * 
 * At a bare minimum a filesystem should support:
 * mount, getdir, readfile
 * 
 * For full read/write support, all endpoints should be defined.
 * 
 * @note The VFS maintains a separate cache of the directory structure
 * which is adjusted to match any requests made to the underlying
 * filesystem driver. The filesystem driver does not have to concern
 * itself with keeping this cache up to date.
 */
typedef struct filesystem_t {

	/**
	 * @brief Name of filesystem driver, e.g. 'fat32'
	 */
	char name[32];

	/**
	 * @brief Function pointer for mount()
	 * Mounts the filesystem to a storage device.
	 */
	mount_volume mount;

	/**
	 * @brief Function pointer for getdir()
	 * Retrieves a list of files in a directory
	 */
	get_directory getdir;

	/**
	 * @brief Function pointer for readfile()
	 * Retrieves file contents from an arbitrary location in
	 * a file on the filesystem. Attempts to read any content
	 * outside of the files extent should be handled by returning
	 * only what is available.
	 * 
	 */
	read_file readfile;

	/**
	 * @brief Function pointer for writefile()
	 * Writes data to arbitrary location in a file on the filesystem.
	 * The file must exist, and writefile() should not create new
	 * files, this is the responsibility of the createfile() endpoint.
	 * 
	 * The writefile() endpoint should extend the length of existing
	 * files where required however, without error and without any
	 * special requirements having to be met, hardware permitting.
	 */
	write_file writefile;

	/**
	 * @brief Function pointer for createfile()
	 * Creates a new file filled with null bytes, of the requested
	 * size. The file size on the media may be larger, to account
	 * for slack, but the reported size must be that provided.
	 * 
	 * The file must not already exist, createfile() should not
	 * overwrite existing data.
	 */
	create_file createfile;

	/**
	 * @brief Function pointer for truncatefile()
	 * Truncates an existing file to a new length, discarding
	 * any data past the new size and freeing it for re-use.
	 * 
	 * The file must exist. To create a new file of a given length,
	 * the createfile() endpoint should be used instead.
	 */
	truncate_file truncatefile;

	/**
	 * @brief Function pointer for createdir()
	 * Create a new empty directory.
	 * The directory should not already exist within the given
	 * parent directory.
	 */
	create_dir createdir;

	/**
	 * @brief Function pointer for rm()
	 * Remove an existing file.
	 * The file should already exist in the parent directory, and
	 * should not itself be a directory.
	 */
	delete_file rm;	

	/**
	 * @brief Function pointer for rmdir()
	 * Remove an existing directory.
	 * The directory should already exist in the parent directory,
	 * and should be empty of all non-special entries (e.g. '.' and '..'
	 * are permitted to still exist)
	 */
	delete_dir rmdir;

	/**
	 * Returns the free space on the filesystem in bytes
	 */
	get_free_space freespace;

	/**
	 * @brief Pointer to next filesystem driver or NULL
	 */
	struct filesystem_t* next;
} filesystem_t;

/**
 * @brief Represents a block storage device e.g. a hard disk, DVD-ROM drive, etc.
 * 
 * A block storage device is expected to provide at least one endpoint for a
 * readonly storage device like a ROM or CD, which is blockread().
 * Reads and writes are always performed in block_size chunks, and any buffers
 * used for requests are expected to be a clean modulus of this block size.
 * 
 * Writeable filesystems should also implement blockwrite().
 */
typedef struct storage_device_t {

	/**
	 * @brief Storage device name, e.g. 'hd0'.
	 * You should use make_unique_device_name() to fill this field.
	 */
	char name[16];

	/**
	 * @brief Total extent of storage device if known,
	 * otherwise this value should be SIZE_MAX
	 */
	uint64_t size;

	/**
	 * @brief Function pointer for blockread()
	 * Block reads are always expected to be in increments of block_size
	 */
	block_read blockread;

	/**
	 * @brief Function pointer for blockwrite()
	 * Block writes are always expected to be in increments of block_size
	 */
	block_write blockwrite;

	/**
	 * @brief The block size read and write operations.
	 * This is usually a sector size on disk drives.
	 */
	uint32_t block_size;

	/**
	 * @brief An opaque integer value which can be given meaning by
	 * the storage device driver.
	 */
	uint64_t opaque1;

	/**
	 * @brief An opaque pointer value which can be given meaning by
	 * the storage device driver.
	 * 
	 */
	void* opaque2;

	/**
	 * @brief Pointer to next storage device, or NULL
	 */
	struct storage_device_t* next;
} storage_device_t;

/**
 * @brief Used internally by filesystem.c to cache directories to RAM.
 * 
 * 
 * Also used to route requests for that directory through to their
 * driver. Also used to attach a driver initially to its mountpoint
 * UNIX-style. fs_tree_t structs are usually not usable or visible to
 * non-filesystem drivers.
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

/**
 * @brief File handle access type
 */
typedef enum fs_handle_type_t {

	/**
	 * @brief Open for input
	 */
	file_input,

	/**
	 * @brief Open for output
	 */
	file_output,

	/**
	 * @brief Open for input and output
	 */
	file_random
} fs_handle_type_t;

/**
 * @brief The data for an open file descriptor.
 * 
 * The FD table is an array of pointers to these structs, any
 * closed FD is NULL.
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


/**
 * @brief Register a new filesystem
 * 
 * @param newfs Pointer to new filesystem information to register
 * @return int nonzero on success
 */
int register_filesystem(filesystem_t* newfs);

/**
 * @brief Find a filesystem by name
 * 
 * @param name Name of filesystem to find
 * @return filesystem_t* Pointer to filesystem if found, or NULL
 */
filesystem_t* find_filesystem(const char* name);

/**
 * @brief Register a new storage device
 * 
 * @param newdev New storage device information to register
 * @return int nonzero on success
 */
int register_storage_device(storage_device_t* newdev);

/**
 * @brief Find a storage device by name
 * 
 * @param name Name of storage device to find
 * @return storage_device_t* Pointer to storage device if found, or NULL
 */
storage_device_t* find_storage_device(const char* name);

/**
 * @brief Read blocks from storage device by name
 * 
 * @param name Name of storage device registered by register_storage_device()
 * @param start_block Starting block number
 * @param bytes Number of bytes to read (should be modulus of block size)
 * @param data Buffer to receive read data
 * @return int nonzero on success
 */
int read_storage_device(const char* name, uint64_t start_block, uint32_t bytes, unsigned char* data);

/**
 * @brief Write blocks to storage device by name
 * 
 * @param name Name of storage device registered by register_storage_device()
 * @param start_block Starting block number
 * @param bytes Number of bytes to write (should be modulus of block size)
 * @param data Data to write
 * @return int nonzero on success
 */
int write_storage_device(const char* name, uint64_t start_block, uint32_t bytes, const unsigned char* data);

/**
 * @brief Attach a filesystem to a VFS directory.
 * Do not use this function for end user features, use filesystem_mount() instead.
 * 
 * @note The opaque data is optional and if included is driver-specific.
 * 
 * @param virtual_path virtual pathname in the VFS
 * @param fs Filesystem driver name
 * @param opaque Opaque data used by the filesystem driver
 * @return int nonzero on success
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

/**
 * @brief Initialise the filesystem
 * This loads the DummyFS filesystem which manages the root directory
 * until any other driver is loaded. DummyFS is a dummy and does nothing.
 */
void init_filesystem();

/**
 * @brief Get a list of files in a directory. The directory path must be fully
 * qualified from the root directory and must contain no trailing slash.
 * 
 * @param pathname fully qualified directory name
 * @return fs_directory_entry_t* linked list of items, or NULL if empty directory
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

/**
 * @brief Retrieve file information on any arbitrary filename.
 * The item requested can be a file, or a directory.
 * @param pathandfile Path to file or directory
 * @return fs_directory_entry_t* information on the file.
 */
fs_directory_entry_t* fs_get_file_info(const char* pathandfile);

/**
 * @brief Create a new empty file
 * 
 * @param pathandfile fully qualified path to new file to create
 * @param bytes size in bytes of file to create
 * @return fs_directory_entry_t* information on new file on success or NULL
 */
fs_directory_entry_t* fs_create_file(const char* pathandfile, size_t bytes);

/**
 * @brief Create a new empty directory
 * 
 * @param pathandfile fully qualified path to new directory to create
 * @param bytes size in bytes of directory to create
 * @return fs_directory_entry_t* information on new directory on success or NULL
 */
fs_directory_entry_t* fs_create_directory(const char* pathandfile);

/**
 * @brief Truncate an existing file to new length
 * @note Any data beyond the new length is discarded and may not
 * be recoverable.
 * 
 * @param file File to truncate
 * @param length New length, should be <= current file size
 * @return int nonzero on success
 */
int fs_truncate_file(fs_directory_entry_t* file, uint32_t length);

/**
 * @brief Read raw bytes from any arbitrary file.
 * 
 * @param file File to read from
 * @param start starting byte position
 * @param length length of data to read, starting at the starting position
 * @param buffer Buffer to receive read data
 * @return int nonzero on error
 */
int fs_read_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer);

uint64_t fs_get_free_space(const char* path);

/**
 * @brief POSIX style _open function
 * 
 * opens a file for read or write access, or creates a new file.
 * 
 * @param filename Filename to create (fully qualified name)
 * @param oflag open state for the file
 * @return int zero on success, -1 on error
 */
int _open(const char *filename, int oflag);

/**
 * @brief POSIX _read function, reads bytes from an open file.
 * 
 * @param fd file descriptor
 * @param buffer buffer to receive data
 * @param count count of bytes to read
 * @return int zero on success, -1 on error
 */
int _read(int fd, void *buffer, unsigned int count);

/**
 * @brief POSIX _write function, writes bytes to an open file.
 * 
 * @param fd file descriptor
 * @param buffer buffer containing data to write
 * @param count count of bytes to write
 * @return int zero on success, -1 on error
 */
int _write(int fd, void *buffer, unsigned int count);

/**
 * @brief POSIX _close function, closes an open file.
 * 
 * @param fd file descriptor
 * @return int zero on success, -1 on error
 */
int _close(uint32_t fd);

/**
 * @brief POSIX _eof function.
 * 
 * reports if we have reached the end of file marker
 * on any open file.
 * 
 * @param fd file descriptor
 * @return int zero if not EOF, 1 if EOF, -1 on error
 */
int _eof(int fd);

/**
 * @brief Seek to given position in a file
 * @note offset + origin should be <= current file size
 * 
 * @param fd file descriptor
 * @param offset offset from origin point
 * @param origin origin point in file
 * @return int64_t new file position
 */
int64_t _lseek(int fd, uint64_t offset, uint64_t origin);

/**
 * @brief Obtain current file position
 * 
 * @param fd file descriptor
 * @return int64_t position in file
 */
int64_t _tell(int fd);

/**
 * @brief Delete a file (not a directory)
 * 
 * @param pathname Fully qualified pathname
 * @return int zero on success, -1 on error
 */
int unlink(const char *pathname);

/**
 * @brief Make a directory
 * 
 * @param pathname Fully qualified pathname
 * @param mode UNIX permission mode, ignored
 * @return int zero on success, -1 on error
 */
int mkdir(const char *pathname, mode_t mode);

/**
 * @brief Remove a directory
 * 
 * @param pathname Fully qualified pathname
 * @return int zero on success, -1 on error
 */
int rmdir(const char *pathname);

/**
 * @brief Truncate a file to the new length
 * @note any data past the new length may not be
 * recoverable.
 * 
 * @param fd file descriptor of an open file
 * @param length New file length
 * @return int zero on success, -1 on error
 */
int ftruncate(int fd, uint32_t length);

/**
 * @brief Low level delete file
 * 
 * @param pathandfile path and filename
 * @return true if file was deleted
 */
bool fs_delete_file(const char* pathandfile);

/**
 * @brief Delete a directory
 * 
 * @param pathandfile fully qualified path to file
 * @return true if directory was deleted
 */
bool fs_delete_directory(const char* pathandfile);

void fs_set_error(uint32_t error);

uint32_t fs_get_error(void);