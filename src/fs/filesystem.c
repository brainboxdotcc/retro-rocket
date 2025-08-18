#include <kernel.h>
#include <filesystem.h>
#include "block_cache.h"

static filesystem_t* filesystems, *dummyfs;
static storage_device_t* storagedevices = NULL;
static fs_tree_t* fs_tree;
static uint32_t fd_last = 0;
static uint32_t fd_alloc = 0;
static fs_handle_t* filehandles[FD_MAX] = { NULL };
static uint32_t fs_last_error[MAX_CPUS] = { FS_ERR_NO_ERROR };

uint8_t verify_path(const char* path);
fs_tree_t* walk_to_node(fs_tree_t* current_node, const char* path);
fs_directory_entry_t* find_file_in_dir(fs_tree_t* directory, const char* filename);
int fs_write_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer);

static const char *fs_error_strings[] = {
	"No error",
	"Operation unsupported",
	"No more file descriptors",
	"Invalid file descriptor",
	"Invalid argument",
	"No such directory",
	"Invalid file path",
	"File already exists",
	"Out of memory",
	"I/O error",
	"Seek past end of file",
	"File not open for output",
	"File not open for input",
	"Truncate beyond current length",
	"No such file",
	"Invalid filename",
	"Cycle detected in directory structure",
	"Broken directory entry",
	"Invalid filesystem geometry",
	"No space left on device",
	"Access outside of volume",
	"Not an RFS volume",
	"Not a file",
	"Invalid VFS data",
	"Not a directory",
	"Directory not empty",
	"File has vanished",
	"Directory already exists",
	"Invalid Primary Volume Descriptor",
	"Invalid Supplementary Volume Descriptor",
	"Oversized directory",
	"Buffer would overflow",
	"No such device",
	"Long filename too long",
	"Filesystem is exFAT",
	"Bad cluster detected",
	"Internal Error",
	"Out of bounds device access",
};

static struct hashmap* vfs_hash = NULL;

typedef struct vfs_tree_t {
	const char* path;
	const char* parent;
	const char** children;
	size_t child_count;
	fs_tree_t* node;
} vfs_tree_t;

typedef struct mountpoint_t {
	const char* path;
	size_t len;
	filesystem_t* responsible_driver;
	struct mountpoint_t* next;
} mountpoint_t;

static mountpoint_t* mountpoints = NULL;

bool vfs_add_mountpoint(const char* mountpoint, filesystem_t* responsible_driver) {
	if (mountpoint == NULL || responsible_driver == NULL) {
		return false;
	}
	mountpoint_t* mount = kmalloc(sizeof(mountpoint_t));
	if (!mount) {
		return false;
	}
	mount->path = strdup(mountpoint);
	if (!mount->path) {
		kfree_null(&mount);
		return false;
	}
	mount->responsible_driver = responsible_driver;

	size_t new_len = strlen(mountpoint);
	mount->len = new_len;

	mountpoint_t** cur = &mountpoints;

	/* Keep list sorted by path length (descending). */
	while (*cur && (*cur)->len >= new_len) {
		cur = &((*cur)->next);
	}

	mount->next = *cur;
	*cur = mount;
	return true;
}


/**
 * Returns the filesystem driver responsible for handling all fs requests to a path
 * and its child objects.
 * @param normalised_path pre-normalised path (can include the filename part)
 * @return filesystem_t* driver ptr, or NULL if no attached driver (only occurs for
 * root before the root filesystem is mounted)
 */
const mountpoint_t* vfs_get_mountpoint_for(const char* normalised_path) {
	if (normalised_path == NULL) {
		return NULL;
	}
	for(mountpoint_t* cur = mountpoints; cur; cur = cur->next) {
		if (strnicmp(normalised_path, cur->path, cur->len) == 0) {
			/* This check ensures that e.g. /programs isnt matched by /prog */
			char next = normalised_path[cur->len];
			if (next == '\0' || next == '/') {
				return cur;
			}
		}
	}
	return NULL;
}

int path_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
	const vfs_tree_t* ua = a;
	const vfs_tree_t* ub = b;
	return strcasecmp(ua->path, ub->path);
}

/* ASCII-only, case-insensitive path hash.
 * Must mirror the semantics of strcasecmp() used in path_compare().
 */
uint64_t path_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const vfs_tree_t* a = (const vfs_tree_t*)item;
	const char* s = a->path;

	/* FNV-1a 64-bit offset basis, perturbed by seed0. */
	uint64_t h = 1469598103934665603ULL ^ seed0;

	for (; *s; ++s) {
		h ^= (unsigned char)tolower(*s);
		h *= 1099511628211ULL; /* FNV prime */
	}

	/* Mix in seed1 and apply a strong finaliser (splitmix64 style). */
	h ^= seed1;
	h ^= h >> 30; h *= 0xbf58476d1ce4e5b9ULL;
	h ^= h >> 27; h *= 0x94d049bb133111ebULL;
	h ^= h >> 31;
	return h;
}


bool vfs_path_add_child(const char* path, const char* child) {
	struct vfs_tree_t* exists = hashmap_get(vfs_hash, &(vfs_tree_t){ .path = path });
	if (!exists) {
		return false;
	}
	if (exists->child_count == 0) {
		exists->children = kmalloc(sizeof(char*));
		exists->children[0] = strdup(child);
		exists->child_count = 1;
	} else {
		for (size_t c = 0; c < exists->child_count; ++c) {
			if (!strcasecmp(child, exists->children[c])) {
				/* Already exists */
				return false;
			}
		}
		void* old = exists->children;
		exists->children = krealloc(exists->children, sizeof(char*) * (exists->child_count + 1));
		if (!exists->children) {
			/* Resize failed */
			exists->children = old;
			return false;
		}
		exists->child_count++;
		exists->children[exists->child_count - 1] = strdup(child);
	}
	return true;
}

vfs_tree_t* find_vfs_path(const char* path) {
	return hashmap_get(vfs_hash, &(vfs_tree_t){ .path = path });
}

bool make_vfs_path(const char* path, fs_tree_t* node) {
	if (path == NULL) {
		return false;
	}
	vfs_tree_t find;
	find.path = strdup(path);
	find.parent = NULL;
	find.node = node;
	find.children = NULL;
	find.child_count = 0;
	struct vfs_tree_t* exists = hashmap_get(vfs_hash, &find);
	if (exists != NULL) {
		kfree_null(&find.path);
		return false;
	} else {
		/* First, split the path and file components */
		size_t namelen = strlen(path);
		char* pathinfo = strdup(path);
		char* pathname = NULL;
		char* ptr = pathinfo + namelen - 1;
		if (namelen > 1 && *ptr == '/') {
			ptr--;
		}
		for (; ptr >= pathinfo; --ptr) {
			if (*ptr == '/') {
				*ptr = 0;
				pathname = (strlen(pathinfo) ? strdup(pathinfo) : strdup("/"));
				break;
			}
		}
		kfree_null(&pathinfo);
		find.parent = pathname;
		void* new_item = hashmap_set(vfs_hash, &find);
		dprintf("VFS tree branch: %s\n", path);
		/* If parent doesn't exist, create it */
		bool parent_is_root = (strcmp(find.parent, "/") == 0);
		if (!find_vfs_path(find.parent)) {
			/* Recurse up the chain making any parents that don't exist */
			if (!parent_is_root && !make_vfs_path(find.parent, NULL)) {
				/* Failure frees this node, and its contained pointers */
				kfree_null(&find.path);
				kfree_null(&find.parent);
				hashmap_delete(vfs_hash, new_item);
				return false;
			}
		}
		/* Add child to parent, now we know it exists for sure */
		if (!parent_is_root) {
			vfs_path_add_child(find.parent, path);
		}
	}
	return true;
}

void init_vfs_tree()
{
	dprintf("Init VFS tree\n");
	vfs_hash = hashmap_new(sizeof(vfs_tree_t), 0, 5648549036, 225546834, path_hash, path_compare, NULL, NULL);
	if (!vfs_hash) {
		preboot_fail("Could not initialise vfs tree hash");
	}
	make_vfs_path("/", fs_tree);

}

char* fs_get_name_part(const char* path) {
	if (!path) {
		return NULL;
	}
	const char* start = path + strlen(path);
	while (start != path && *start != '/') {
		start--;
	}
	return strdup(start + 1);
}

void fs_set_error(uint32_t error) {
	fs_last_error[logical_cpu_id()] = error;
}

uint32_t fs_get_error(void) {
	return fs_last_error[logical_cpu_id()];
}

int register_filesystem(filesystem_t* newfs)
{
	/* Add the new filesystem to the start of the list */
	newfs->next = filesystems;
	filesystems = newfs;
	return 1;
}

filesystem_t* find_filesystem(const char* name)
{
	filesystem_t* cur = filesystems;
	for(; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}


int register_storage_device(storage_device_t* newdev)
{
	/* Add the new storage device to the start of the list */
	dprintf("Registered block storage device '%s'\n", newdev->name);
	newdev->next = storagedevices;
	storagedevices = newdev;
	return 1;
}

bool storage_enable_cache(storage_device_t* device) {
	if (!device || device->cache || !device->blockread) {
		dprintf("Invalid device state for cache enable\n");
		return false;
	}
	device->cache = block_cache_create(device);
	return device->cache != NULL;
}


void storage_disable_cache(storage_device_t *sd) {
	if (!sd || !sd->cache) {
		return;
	}
	block_cache_destroy(&sd->cache);
}

storage_device_t* find_storage_device(const char* name)
{
	storage_device_t* cur = storagedevices;
	for(; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

const storage_device_t* get_all_storage_devices() {
	return storagedevices;
}

uint64_t fs_get_free_space(const char* path)
{
	fs_tree_t* dir = walk_to_node(fs_tree, path);
	return dir && dir->responsible_driver && dir->responsible_driver->freespace ? dir->responsible_driver->freespace(dir) : 0;
}

int read_storage_device(const char* name, uint64_t start_block, uint32_t bytes, unsigned char* data)
{
	storage_device_t* cur = find_storage_device(name);
	if (!cur || !cur->blockread) {
		fs_set_error(FS_ERR_UNSUPPORTED);
		return 0;
	}

	if (cur->cache) {
		if (block_cache_read(cur->cache, start_block, bytes, data)) {
			return 1;
		}

		if (!cur->blockread(cur, start_block, bytes, data)) {
			fs_set_error(FS_ERR_IO);
			return 0;
		}

		/* Prime the cache with what we just read; ignore result */
		block_cache_write(cur->cache, start_block, bytes, data);
		return 1;
	}

	if (!cur->blockread(cur, start_block, bytes, data)) {
		fs_set_error(FS_ERR_IO);
		return 0;
	}

	return 1;
}

int write_storage_device(const char* name, uint64_t start_block, uint32_t bytes, const unsigned char* data)
{
	storage_device_t* cur = find_storage_device(name);
	if (!cur || !cur->blockwrite) {
		fs_set_error(FS_ERR_UNSUPPORTED);
		return 0;
	}

	/* Always write the device first */
	if (!cur->blockwrite(cur, start_block, bytes, data)) {
		fs_set_error(FS_ERR_IO);
		return 0;
	}

	/* Then, if present, update/prime the cache (no I/O here) */
	if (cur->cache) {
		block_cache_write(cur->cache, start_block, bytes, data);
	}

	return 1;
}

/* Allocate a new file descriptor and attach it to 'file' */
int alloc_filehandle(fs_handle_type_t type, fs_directory_entry_t* file, uint32_t ibufsz, uint32_t obufsz)
{
	/* Check we haven't used up all available fds */
	if (fd_alloc >= FD_MAX) {
		fs_set_error(FS_ERR_NO_MORE_FDS);
		return -1;
	}

	uint32_t iter = 0;
	while (iter++ < FD_MAX) {
		/* Found an empty slot */
		if (filehandles[fd_last] == NULL) {
			/* Initialise fs_handle_t struct */
			filehandles[fd_last] = kmalloc(sizeof(fs_handle_t));
			if (!filehandles[fd_last]) {
				return -1;
			}
			filehandles[fd_last]->type = type;
			filehandles[fd_last]->file = file;
			filehandles[fd_last]->seekpos = 0;

			filehandles[fd_last]->inbufsize = ibufsz;
			filehandles[fd_last]->outbufsize = obufsz;
			filehandles[fd_last]->inbufpos = 0;
			filehandles[fd_last]->outbufpos = 0;
			if (ibufsz) {
				filehandles[fd_last]->inbuf = kmalloc(ibufsz);
			} else {
				filehandles[fd_last]->inbuf = NULL;
			}
			if (obufsz) {
				filehandles[fd_last]->outbuf = kmalloc(obufsz);
			} else {
				filehandles[fd_last]->outbuf = NULL;
			}

			fd_alloc++;
			int ret = (int)fd_last;
			fd_last++;
			return ret;
		} else {
			fd_last++;

			/* Reached the end of the list? loop back around */
			if (fd_last >= FD_MAX) {
				fd_last = 0;
			}
		}
	}
	fs_set_error(FS_ERR_NO_MORE_FDS);
	return -1;
}

/* Free a file descriptor */
uint32_t destroy_filehandle(uint32_t descriptor)
{
	dprintf("destroy_filehandle(%d)\n", descriptor);
	/* Sanity checks */
	if (descriptor >= FD_MAX || filehandles[descriptor] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
		return 0;
	} else {
		filehandles[descriptor]->file = NULL;
		filehandles[descriptor]->seekpos = 0;
		if (filehandles[descriptor]->inbuf) {
			kfree_null(&filehandles[descriptor]->inbuf);
		}
		if (filehandles[descriptor]->outbuf) {
			kfree_null(&filehandles[descriptor]->outbuf);
		}
		kfree_null(&filehandles[descriptor]);
		/* Make the search faster for the next process to ask
		 * for an fd, and try to keep some semblence of POSIX
		 * compliance by making this fd immediately available
		 * as the next fd if it is lower than what we're handing
		 * out next.
		 */
		if (descriptor < fd_last)
			fd_last = descriptor;
		fd_alloc--;
	}
	return 1;
}

fs_directory_entry_t* fs_create_file(const char* pathandfile, size_t bytes)
{
	fs_directory_entry_t* new_entry = NULL;
	if (!pathandfile) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	dprintf("fs_create_file '%s'\n", pathandfile);

	if (!verify_path(pathandfile)) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		return false;
	}

	/* First, split the path and file components */
	uint32_t namelen = strlen(pathandfile);
	char* pathinfo = strdup(pathandfile);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strdup(pathinfo);
			break;
		}
	}
	kfree_null(&pathinfo);

	dprintf("vfs create: filename: %s pathname: %s\n", filename, pathname);

	if (!filename || !pathname || !*filename) {
		fs_set_error(FS_ERR_INVALID_FILEPATH);
		return false;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		kfree_null(&pathname);
		kfree_null(&filename);
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		return false;
	}

	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (fileinfo) {
		dprintf("vfs create: file in %s already exists: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
		fs_set_error(FS_ERR_FILE_EXISTS);
		return false;
	}
	
	if (directory->responsible_driver && directory->responsible_driver->createfile) {
		uint64_t lbapos = directory->responsible_driver->createfile(directory, filename, bytes);
		/* Remove the deleted file from the fs_tree_t */
		if (lbapos) {
			new_entry = kmalloc(sizeof(fs_directory_entry_t));
			if (!new_entry) {
				kfree_null(&pathname);
				kfree_null(&filename);
				fs_set_error(FS_ERR_OUT_OF_MEMORY);
				return false;
			}
			datetime_t dt;
			get_datetime(&dt);
			get_weekday_from_date(&dt);
			new_entry->device = 0;
			strlcpy(new_entry->device_name, directory->responsible_driver->name, 15);
			new_entry->directory = directory;
			new_entry->filename = strdup(filename);
			new_entry->alt_filename = strdup(filename);
			new_entry->flags = 0;
			new_entry->lbapos = lbapos;
			new_entry->day = dt.day;
			new_entry->month = dt.month;
			new_entry->year = (dt.century - 1) * 100 + dt.year;
			new_entry->hour = dt.hour;
			new_entry->min = dt.minute;
			new_entry->sec = dt.second;
			new_entry->next = directory->files;
			new_entry->size = bytes;
			directory->files = new_entry;
		}
	}
	kfree_null(&pathname);
	kfree_null(&filename);
	return new_entry;
}

fs_directory_entry_t* fs_create_directory(const char* pathandfile)
{
	fs_directory_entry_t* new_entry = NULL;

	if (!verify_path(pathandfile)) {
		dprintf("fs_create_directory: verify path and file failed: '%s'\n", pathandfile);
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		return false;
	}

	/* First, split the path and file components */
	uint32_t namelen = strlen(pathandfile);
	char* pathinfo = strdup(pathandfile);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strdup(pathinfo);
			break;
		}
	}
	kfree_null(&pathinfo);

	if (!filename || !pathname || !*filename) {
		dprintf("Invalid filepath\n");
		fs_set_error(FS_ERR_INVALID_FILEPATH);
		return false;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		dprintf("fs_create_directory: walk_to_node '%s' failed\n", pathname);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}

	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (fileinfo) {
		dprintf("vfs create dir: file in %s already exists: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
		fs_set_error(FS_ERR_FILE_EXISTS);
		return false;
	}
	
	if (directory->responsible_driver && directory->responsible_driver->createdir) {
		uint64_t lbapos = directory->responsible_driver->createdir(directory, filename);
		/* Remove the deleted file from the fs_tree_t */
		if (lbapos) {
			dprintf("Driver createdirectory %lu\n", lbapos);
			new_entry = kmalloc(sizeof(fs_directory_entry_t));
			datetime_t dt;
			get_datetime(&dt);
			get_weekday_from_date(&dt);
			new_entry->device = 0;
			strlcpy(new_entry->device_name, directory->responsible_driver->name, 15);
			new_entry->directory = directory;
			new_entry->filename = strdup(filename);
			new_entry->alt_filename = strdup(filename);
			new_entry->lbapos = lbapos;
			new_entry->day = dt.day;
			new_entry->month = dt.month;
			new_entry->year = (dt.century - 1) * 100 + dt.year;
			new_entry->hour = dt.hour;
			new_entry->min = dt.minute;
			new_entry->sec = dt.second;
			new_entry->next = directory->files;
			new_entry->size = 0;
			new_entry->flags = FS_DIRECTORY;
			directory->files = new_entry;
			/* TODO add to FS tree, dont forget responsible_driver ptr! */
			fs_tree_t* new_dir  = kmalloc(sizeof(fs_tree_t));
			new_dir->next = directory->child_dirs;
			new_dir->device = directory->device;
			strlcpy(new_dir->device_name, directory->device_name, 16);
			new_dir->dirty = 1;
			new_dir->files = NULL;
			new_dir->lbapos = lbapos;
			new_dir->name = strdup(filename);
			new_dir->child_dirs = NULL;
			new_dir->parent = directory;
			new_dir->opaque = directory->opaque;
			new_dir->responsible_driver = directory->responsible_driver;
			new_dir->size = 0;
			directory->child_dirs = new_dir;
		} else {
			dprintf("driver createdirectory failed\n");
		}
	}
	kfree_null(&pathname);
	kfree_null(&filename);
	return new_entry;
}

int mkdir(const char *pathname, [[maybe_unused]] mode_t mode)
{
	return fs_create_directory(pathname) ? 0 : -1;
}


/* Open a file for access */
int _open(const char* filename, int oflag)
{
	fs_handle_type_t type = file_input;
	fs_directory_entry_t* file = NULL;
	/* First check if we can find the file in the filesystem */

	if ((oflag & _O_APPEND) == _O_APPEND) {
		type = file_random;
	} else if ((oflag & _O_CREAT) == _O_CREAT) {
		type = file_output;
	} else if ((oflag & _O_RDWR) == _O_RDWR) {
		type = file_random;
	} else if ((oflag & _O_WRONLY) == _O_WRONLY) {
		type = file_output;
	} else {
		type = file_input;
	}

	dprintf("_open type: %d for file %s\n", type, filename);

	file = fs_get_file_info(filename);
	if (file == NULL && type == file_input) {
		dprintf("open for input does not exist: %s\n", filename);
		fs_set_error(FS_ERR_INVALID_FILEPATH);
		return -1;
	} else if (file == NULL && type != file_input) {
		file = fs_create_file(filename, 0);
	} else if (file != NULL && type == file_output) {
		fs_truncate_file(file, 0);
		dprintf("Open for output ONLY, existing file; truncated to 0\n");
	}
	if (file == NULL) {
		return -1;
	}

	dprintf("_open file info obtained\n");

	/* Allocate a file handle.
	 */
	int fd = alloc_filehandle(type, file, IOBUFSZ, 0);
	if (fd == -1) {
		fs_set_error(FS_ERR_NO_MORE_FDS);
		return -1;
	}

	dprintf("_open alloc filehandle %d\n", fd);

	filehandles[fd]->cached = 0;
	if (type == file_random || type == file_input) {
		dprintf("read into buffer pos=%ld sz=%ld buf=%lx\n", filehandles[fd]->seekpos, file->size <= filehandles[fd]->inbufsize ? file->size : filehandles[fd]->inbufsize, (uint64_t)filehandles[fd]->inbuf);
		/* Read an initial buffer into the structure up to fd->inbufsize in size */
		if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos, file->size <= filehandles[fd]->inbufsize ? file->size : filehandles[fd]->inbufsize, filehandles[fd]->inbuf)) {
			/* If we couldn't get the initial buffer, there is something wrong.
			* Give up the filehandle and return error.
			*/
			destroy_filehandle(fd);
			return -1;
		} else {
			if (file->size <= filehandles[fd]->inbufsize) {
				filehandles[fd]->cached = 1;
			}
		}
	}

	dprintf("_open done\n");

	/* Return the allocated file descriptor */
	return fd;
}

/* Write out buffered data to a file open for writing */
void flush_filehandle([[maybe_unused]] uint32_t descriptor)
{
	/* Until we have writeable filesystems this is a stub */
}

/* Close an open file descriptor */
int _close(uint32_t descriptor)
{
	dprintf("_close(%d)\n", descriptor);
	/* Sanity checks */
	if (descriptor >= FD_MAX || filehandles[descriptor] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
		return -1;
	}

	/* Flush any files that arent readonly */
	if (filehandles[descriptor]->type != file_input) {
		flush_filehandle(descriptor);
	}

	return destroy_filehandle(descriptor) ? 0 : -1;
}

long _lseek(int fd, uint64_t offset, uint64_t origin)
{
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		return -1;
	} else {
		if (offset + origin > filehandles[fd]->file->size) {
			/* Do not allow seeking past end */
			fs_set_error(FS_ERR_SEEK_PAST_END);
			return -1;
		} else {
			filehandles[fd]->seekpos = offset + origin;
			/* Flush output before seeking */
			flush_filehandle(fd);
			/* Refresh input buffer */
			if (filehandles[fd]->type != file_output && filehandles[fd]->seekpos < filehandles[fd]->file->size) {
				if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos, filehandles[fd]->inbufsize, filehandles[fd]->inbuf)) {
					return -1;
				}
			}
			return filehandles[fd]->seekpos;
		}
	}
	return -1;
}

int64_t _tell(int fd)
{
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
	}
	return (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) ? (int64_t)-1 : (int64_t)filehandles[fd]->seekpos; 
}

/* Read bytes from an open file */
int _read(int fd, void *buffer, unsigned int count)
{
	/* Sanity checks */
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
		return -1;
	}

	/* can't read from a write-only handle */
	if (filehandles[fd]->type == file_output) {
		fs_set_error(FS_ERR_NOT_OPEN_FOR_INPUT);
		return -1;
	}

	if (filehandles[fd]->seekpos >= filehandles[fd]->file->size) {
		return 0;
	}

	/* Check that the size of the request and current position
	 * don't place any part of the buffer past the bounds of the file
	 */
	if ((filehandles[fd]->seekpos + count) > filehandles[fd]->file->size) {
		/* Request too large, truncate it to EOF */
		count = filehandles[fd]->file->size - filehandles[fd]->seekpos;
	}

	if (((filehandles[fd]->seekpos % filehandles[fd]->inbufsize) + count) > filehandles[fd]->inbufsize) {
		/* The requested buffer size is too large to read in one operation.
		 * Continually read into the input buffer in filehandles[fd]->inbufsize
		 * chunks maximum until all data is read.
		 */
		int readbytes = 0;
		while (count > 0) {
			int rb;
			if (count > filehandles[fd]->inbufsize) {
				rb = filehandles[fd]->inbufsize;
			} else {
				rb = count;
			}
			if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos, rb, filehandles[fd]->inbuf)) {
				return -1;
			}

			memcpy(((unsigned char*)buffer) + readbytes, filehandles[fd]->inbuf, rb);

			filehandles[fd]->seekpos += rb;
			readbytes += rb;
			count -= rb;
		}
		count = readbytes;


	} else {
		/* we can do the entire read from only the current IO buffer */

		/* Read the entire lot in one go */
		if (filehandles[fd]->cached == 0) {
			if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos, count, filehandles[fd]->inbuf)) {
				return -1;
			}
			memcpy(buffer, filehandles[fd]->inbuf, count);
		} else {
			memcpy(buffer, filehandles[fd]->inbuf + filehandles[fd]->seekpos, count);
		}

		filehandles[fd]->seekpos += count;
	}

	return count;
}

int ftruncate(int fd, uint32_t length)
{
	/* Sanity checks */
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
		return -1;
	}

	/* can't truncate a read-only handle */
	if (filehandles[fd]->type == file_input) {
		fs_set_error(FS_ERR_NOT_OPEN_FOR_OUTPUT);
		return -1;
	}

	if (length > filehandles[fd]->file->size) {
		fs_set_error(FS_ERR_TRUNCATE_BEYOND_LENGTH);
		return -1;
	}

	dprintf("truncate() fs_truncate_file %d\n", length);
	if (!fs_truncate_file(filehandles[fd]->file, length)) {
		return -1;
	}

	filehandles[fd]->file->size = length;

	if (filehandles[fd]->seekpos > filehandles[fd]->file->size) {
		filehandles[fd]->seekpos = filehandles[fd]->file->size;
	}

	return 0;
}


/* Write bytes to an open file */
int _write(int fd, void *buffer, unsigned int count)
{
	dprintf("_write()\n");
	/* Sanity checks */
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
		return -1;
	}

	/* can't write to a read-only handle */
	if (filehandles[fd]->type == file_input) {
		fs_set_error(FS_ERR_NOT_OPEN_FOR_OUTPUT);
		return -1;
	}

	dprintf("_write() fs_write_file %u\n", count);
	if (!fs_write_file(filehandles[fd]->file, filehandles[fd]->seekpos, count, buffer)) {
		return -1;
	}

	if (filehandles[fd]->seekpos >= filehandles[fd]->file->size) {
		/* Underlying driver will extend file too */
		dprintf("_write growing file from %lu to %lu\n", filehandles[fd]->file->size, filehandles[fd]->seekpos + count);
		filehandles[fd]->file->size = filehandles[fd]->seekpos + count;
	}

	filehandles[fd]->seekpos += count;
	dprintf("_write done\n");
	return count;
}

int _eof(int fd)
{
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		fs_set_error(FS_ERR_INVALID_FD);
	}
	return (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) ? -1 : (filehandles[fd]->seekpos >= filehandles[fd]->file->size);
}

void retrieve_node_from_driver(fs_tree_t* node)
{
	/* XXX: Check there isnt already content in node->files, if there is,
	 * delete the old content first to avoid a memleak.
	 */

	dprintf("retrieve_node_from_driver\n");
	if (node == NULL) {
		return;
	}

	filesystem_t* driver = (filesystem_t*)node->responsible_driver;

	if (driver == dummyfs) {
		return;
	}

	if (driver == NULL || driver->getdir == NULL) {
		/* Driver does not implement getdir() */
		kprintf("*** BUG *** Driver '%s' on node '%s' is null or does not support getdir()!\n", driver ? driver->name : "NULL", node->name);
		return;
	}

	node->files = driver->getdir(node);
	node->dirty = 0;
	node->child_dirs = NULL;

	if (node->files == NULL) {
		node->parent = node;
		return;
	}
	/* XXX: For directories that already contain items,
	 * enumerate the list of directories, take note of the
	 * responsible driver for this directory, and reuse these
	 * responsible driver pointers on the new entries if they
	 * crop up again. If we don't do this, then we will claim
	 * ownership of another filesystems mountpoint!
	 */
	fs_directory_entry_t* x = (fs_directory_entry_t*)node->files;
	for (; x; x = x->next) {
		//kprintf("Parse entry '%s'@%08x\n", x->filename, x);
		if (x->flags & FS_DIRECTORY) {
			/* Insert a new child directory into node->child_dirs,
			 * Make each dir empty and 'dirty' and get its opaque
			 * from the parent dir
			 */
			fs_tree_t* newnode = kmalloc(sizeof(fs_tree_t));
			if (!newnode) {
				return;
			}

			newnode->name = x->filename;
			newnode->lbapos = x->lbapos;
			newnode->size = x->size;
			newnode->device = x->device;
			strlcpy(newnode->device_name, x->device_name, 16);
			newnode->opaque = node->opaque;
			newnode->dirty = 1;
			newnode->parent = node;
			newnode->responsible_driver = node->responsible_driver;

			newnode->next = node->child_dirs;
			node->child_dirs = newnode;
		}
	}
}

typedef struct dirstack_t
{
	char* name;
	struct dirstack_t* next;
} dirstack_t;

fs_tree_t* walk_to_node_internal(fs_tree_t* current_node, dirstack_t* dir_stack)
{
	if (!current_node) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	/*if (current_node->dirty != 0) {
		retrieve_node_from_driver(current_node);
	}

	if (dir_stack && current_node->name != NULL && dir_stack->name != NULL && !strcmp(current_node->name, dir_stack->name)) {
		dir_stack = dir_stack->next;
		if (!dir_stack) {
			return current_node;
		}
	}

	fs_tree_t* dirs = current_node->child_dirs;
	for (; dirs; dirs = dirs->next) {
		fs_tree_t* result = walk_to_node_internal(dirs, dir_stack);
		if (result != NULL) {
			return result;
		}
	}

	return NULL;*/


	/* Materialise this node before inspecting children */
	if (current_node->dirty != 0) {
		retrieve_node_from_driver(current_node);
	}

	/* No more segments to consume -> we've arrived */
	if (dir_stack == NULL) {
		return current_node;
	}

	/* IMPORTANT CHANGE:
	   Only consider immediate children named like the next segment.
	   Do NOT recurse into non-matching branches (prevents /a from
	   “finding” /x/.../a by DFS). */
	for (fs_tree_t *child = current_node->child_dirs; child; child = child->next) {
		if (child->name && dir_stack->name && strcmp(child->name, dir_stack->name) == 0) {
			/* consume exactly one segment and descend */
			return walk_to_node_internal(child, dir_stack->next);
		}
	}

	/* No matching child at this level */
	return NULL;
}

uint8_t verify_path(const char* path)
{
	if (!path || !*path) {
		return 0;
	}
	/* Valid paths must start with a / and not end with a / */
	/* The filesystem internals all work with fully qualified
	 * pathnames. Client processes must represent 'current'
	 * directories to the user if they are required.
	 */
	uint32_t pathlen = strlen(path);
	return ((*path == '/' && *(path + pathlen - 1) != '/') || !strcmp(path, "/"));
}

fs_tree_t* walk_to_node(fs_tree_t* current_node, const char* path)
{
	if (!verify_path(path)) {
		dprintf("walk_to_node: Verify path failed: %s\n", path);
		return NULL;
	}
	if (!strcmp(path, "/")) {
		dprintf("walk_to_node: returning root of tree: %s\n", path);
		return fs_tree;
	}
	/* First build the dir stack */
	dirstack_t* ds = kmalloc(sizeof(dirstack_t));
	if (!ds) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	dirstack_t* walk = ds;
	char* copy = strdup(path);
	char* parse;
	char* last = copy + 1;

	for (parse = copy + 1; *parse; ++parse) {
		if (*parse == '/') {
			*parse = 0;
			walk->name = strdup(last);
			dprintf("Dirstack part: '%s'\n", walk->name);
			last = parse + 1;

                        dirstack_t* next = kmalloc(sizeof(dirstack_t));
			if (!next) {
				break;
			}
			walk->next = next;
			next->next = 0;
			next->name = NULL;
			walk = next;
		}
	}
	walk->next = NULL;
	walk->name = strdup(last);
	dprintf("Made dirstack: '%s'\n", path);
	fs_tree_t* result = walk_to_node_internal(current_node, ds);
	dprintf("Walked nodes, got result %p\n", result);
	while (ds) {
		dirstack_t* next = ds->next;
		kfree_null(&ds->name);
		kfree_null(&ds);
		ds = next;
	}
	return result;
}

fs_directory_entry_t* find_file_in_dir(fs_tree_t* directory, const char* filename)
{
	if (!directory || !filename) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	fs_directory_entry_t* entry = (fs_directory_entry_t*)directory->files;
	for (; entry; entry = entry->next) {
		/* Don't find directories, only files */
		if (((entry->flags & FS_DIRECTORY) == 0) && (!strcmp(filename, entry->filename))) {
			return entry;
		}
	}
	return NULL;
}

fs_directory_entry_t* find_dir_in_dir(fs_tree_t* directory, const char* filename)
{
	if (!directory || !filename) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}

	fs_directory_entry_t* entry = (fs_directory_entry_t*)directory->files;
	for (; entry; entry = entry->next) {
		/* Don't find directories, only files */
		if ((entry->flags & FS_DIRECTORY) && (!strcmp(filename, entry->filename)))
			return entry;
	}
	return NULL;
}



int fs_read_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer)
{
	if (file && buffer && file->directory && file->directory->responsible_driver && file->directory->responsible_driver->readfile) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs ? fs->readfile(file, start, length, buffer) : 0;
	}
	fs_set_error(!file || !buffer ? FS_ERR_INVALID_ARG : FS_ERR_UNSUPPORTED);
	return 0;
}

int fs_write_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer)
{
	if (file && buffer && file->directory && file->directory->responsible_driver && file->directory->responsible_driver->writefile) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs && fs->writefile ? fs->writefile(file, start, length, buffer) : 0;
	}
	fs_set_error(!file || !buffer ? FS_ERR_INVALID_ARG : FS_ERR_UNSUPPORTED);
	return 0;
}

int fs_truncate_file(fs_directory_entry_t* file, uint32_t length)
{
	if (file && file->directory && file->directory->responsible_driver && file->directory->responsible_driver->truncatefile) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs && fs->truncatefile ? fs->truncatefile(file, length) : 0;
	}
	fs_set_error(!file ? FS_ERR_INVALID_ARG : FS_ERR_UNSUPPORTED);
	return 0;
}

void delete_tree_node(fs_tree_t** head_ref, const char* name)
{
	if (head_ref == NULL) {
		return;
	}

	fs_tree_t *temp = *head_ref, *prev = NULL;

	if (temp != NULL && !strcmp(temp->name, name)) {
		*head_ref = temp->next;
		kfree_null(&temp);
		return;
	}

	while (temp != NULL && strcmp(temp->name, name) != 0) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL) {
		return;
	}
	if (prev) {
		prev->next = temp->next;
	}
	kfree_null(&temp);
}

void delete_file_node(fs_directory_entry_t** head_ref, const char* name)
{
	if (head_ref == NULL) {
		return;
	}

	fs_directory_entry_t *temp = *head_ref, *prev = NULL;

	if (temp != NULL && !strcmp(temp->filename, name)) {
		*head_ref = temp->next;
		kfree_null(&temp);
		return;
	}

	while (temp != NULL && strcmp(temp->filename, name) != 0) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL) {
		return;
	}

	if (prev) {
		prev->next = temp->next;
	}
	kfree_null(&temp);
}

bool fs_delete_file(const char* pathandfile)
{
	if (!verify_path(pathandfile)) {
		fs_set_error(FS_ERR_INVALID_FILEPATH);
		return false;
	}

	/* First, split the path and file components */
	uint32_t namelen = strlen(pathandfile);
	char* pathinfo = strdup(pathandfile);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strdup(pathinfo);
			break;
		}
	}
	kfree_null(&pathinfo);

	if (!filename || !pathname || !*filename) {
		return false;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (!fileinfo) {
		fs_set_error(FS_ERR_NO_SUCH_FILE);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	
	bool rv = false;
	if (!(fileinfo->flags & FS_DIRECTORY)) {
		if (directory->responsible_driver && directory->responsible_driver->rm) {
			rv = directory->responsible_driver->rm(directory, filename);
			/* Remove the deleted file from the fs_tree_t */
			if (rv) {
				delete_file_node(&(directory->files), filename);
			}
		} else {
			fs_set_error(FS_ERR_UNSUPPORTED);
		}
	} else {
		fs_set_error(FS_ERR_NOT_A_FILE);
	}
	kfree_null(&pathname);
	kfree_null(&filename);
	return rv;
}

bool fs_delete_directory(const char* pathandfile)
{
	if (!verify_path(pathandfile)) {
		fs_set_error(FS_ERR_INVALID_FILEPATH);
		return false;
	}

	/* First, split the path and file components */
	uint32_t namelen = strlen(pathandfile);
	char* pathinfo = strdup(pathandfile);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strdup(pathinfo);
			break;
		}
	}
	kfree_null(&pathinfo);

	if (!filename || !pathname || !*filename) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	fs_directory_entry_t* fileinfo = find_dir_in_dir(directory, filename);
	if (!fileinfo) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	
	bool rv = false;
	if (fileinfo->flags & FS_DIRECTORY) {
		if (directory->responsible_driver && directory->responsible_driver->rmdir) {
			rv = directory->responsible_driver->rmdir(directory, filename);
			/* Remove the deleted file from the fs_tree_t */
			if (rv) {
				delete_file_node(&(directory->files), filename);
				delete_tree_node(&(directory->child_dirs), filename);
			}
		} else {
			fs_set_error(FS_ERR_UNSUPPORTED);
		}
	} else {
		fs_set_error(FS_ERR_NOT_A_DIRECTORY);
	}
	kfree_null(&pathname);
	kfree_null(&filename);
	return rv;
}


int unlink(const char *pathname)
{
	return (fs_delete_file(pathname) ? 0 : -1);
}

int rmdir(const char *pathname)
{
	return (fs_delete_directory(pathname) ? 0 : -1);
}

fs_directory_entry_t* fs_get_file_info(const char* pathandfile)
{
	if (!verify_path(pathandfile)) {
		return NULL;
	}

	/* First, split the path and file components */
	uint32_t namelen = strlen(pathandfile);
	char* pathinfo = strdup(pathandfile);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strdup(pathinfo);
			break;
		}
	}
	kfree_null(&pathinfo);

	if (!filename || !pathname || !*filename) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return NULL;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		kfree_null(&pathname);
		kfree_null(&filename);
		return NULL;
	}
	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);
	kfree_null(&pathname);
	kfree_null(&filename);
	return fileinfo;
}

int attach_filesystem(const char* virtual_path, filesystem_t* fs, void* opaque)
{
	fs_tree_t* item = walk_to_node(fs_tree, virtual_path);
	make_vfs_path(virtual_path, item);
	vfs_add_mountpoint(virtual_path, fs);
	if (item == NULL) {
		fs_set_error(FS_ERR_NO_SUCH_DIRECTORY);
		return 0;
	}
	item->responsible_driver = (void*)fs;
	item->name = !strcmp(virtual_path, "/") ? strdup("/") : fs_get_name_part(virtual_path);
	dprintf("Attach virtual path '%s' -> '%s'", virtual_path, item->name ? item->name : "<NULL>");
	item->opaque = opaque;
	item->dirty = 1;
	item->files = NULL;
	item->child_dirs = NULL;
	item->lbapos = 0; // special value always refers to root dir
	retrieve_node_from_driver(item);
	return 1;
}

void init_filesystem()
{
	init_vfs_tree();
	filesystems = kmalloc(sizeof(filesystem_t));
	if (!filesystems) {
		preboot_fail("Unable to allocate memory for filesystem list");
	}
	fs_tree = kmalloc(sizeof(fs_tree_t));
	if (!fs_tree) {
		preboot_fail("Unable to allocate memory for vfs tree");
	}

	strlcpy(filesystems->name, "DummyFS", 31);
	filesystems->mount = NULL;
	filesystems->getdir = NULL;
	filesystems->readfile = NULL;
	filesystems->writefile = NULL;
	filesystems->rm = NULL;
	filesystems->createfile = NULL;
	filesystems->createdir = NULL;
	filesystems->truncatefile = NULL;
	filesystems->rmdir = NULL;
	filesystems->freespace = NULL;
	filesystems->next = NULL;

	dummyfs = filesystems;

	fs_tree->name = NULL;
	fs_tree->parent = fs_tree;
	fs_tree->child_dirs = NULL;
	fs_tree->files = NULL;
	fs_tree->dirty = 0;
	fs_tree->opaque = NULL;

	attach_filesystem("/", filesystems, NULL);
}

fs_directory_entry_t* fs_get_items(const char* pathname)
{
	fs_tree_t* item = walk_to_node(fs_tree, pathname);
	make_vfs_path(pathname, item);
	return (fs_directory_entry_t*)(item ? item->files : NULL);
}

bool fs_is_directory(const char* path)
{
	if (strcmp(path, "/") == 0) {
		return true;
	}
	/* Get the containing directory of the item in 'pathname' */

	/* First, split the path and file components */
	uint32_t namelen = strlen(path);
	char* pathinfo = strdup(path);
	char* filename = NULL;
	char* pathname = NULL;
	char* ptr;
	for (ptr = pathinfo + namelen; ptr >= pathinfo; --ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			filename = strdup(ptr + 1);
			pathname = strlen(pathinfo) ? strdup(pathinfo) : strdup("/");
			break;
		}
	}
	kfree_null(&pathinfo);

	dprintf("FS_IS_DIRECTORY checking path='%s', pathpart='%s' filepart='%s'\n", path, pathname, filename);

	fs_tree_t* item = walk_to_node(fs_tree, pathname);
	if (item == NULL) {
		dprintf("FS_IS_DIRECTORY: WALK TO NODE '%s' FAILED\n", pathname);
		kfree_null(&filename);
		kfree_null(&pathname);
		return false;
	}
	for (fs_directory_entry_t* ent = item->files; ent; ent = ent->next) {
		if (strcasecmp(ent->filename, filename) == 0) {
			dprintf("FS_IS_DIRECTORY: path='%s' found file='%s' with flags '%u'\n", pathname, ent->filename, ent->flags);
			kfree_null(&filename);
			kfree_null(&pathname);
			return (ent->flags & FS_DIRECTORY) != 0;
		}
	}
	kfree_null(&filename);
	kfree_null(&pathname);
	return false;
}

int filesystem_mount(const char* pathname, const char* device, const char* filesystem_driver)
{
	dprintf("filesystem_mount(%s, %s, %s)\n", pathname, device ? device : "(null)", filesystem_driver);
	filesystem_t *driver = find_filesystem(filesystem_driver);
	int success = (driver && driver->mount(device, pathname));
	kprintf(
		"%s %s to %s%s%s\n",
		success ? "Mounted" : "Failed to mount",
		pathname,
		device && *device ? device : "",
		device && *device ? " as " : "",
		filesystem_driver
	);
	return success;

}

const char *fs_strerror(fs_error_t err) {
	if (err >= 0 && err < (sizeof(fs_error_strings) / sizeof(fs_error_strings[0]))) {
		return fs_error_strings[err];
	}
	return "Unknown filesystem error";
}
