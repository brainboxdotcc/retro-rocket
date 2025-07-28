#include <kernel.h>
#include <filesystem.h>

filesystem_t* filesystems, *dummyfs;
storage_device_t* storagedevices = NULL;
fs_tree_t* fs_tree;
static uint32_t fd_last = 0;
static uint32_t fd_alloc = 0;
fs_handle_t* filehandles[FD_MAX] = { NULL };

uint8_t verify_path(const char* path);
fs_tree_t* walk_to_node(fs_tree_t* current_node, const char* path);
fs_directory_entry_t* find_file_in_dir(fs_tree_t* directory, const char* filename);
int fs_write_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer);

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

int read_storage_device(const char* name, uint64_t start_block, uint32_t bytes, unsigned char* data)
{
	storage_device_t* cur = find_storage_device(name);
	if (!cur || !cur->blockread) {
		return 0;
	}
	return cur->blockread(cur, start_block, bytes, data);
}

int write_storage_device(const char* name, uint64_t start_block, uint32_t bytes, const unsigned char* data)
{
	storage_device_t* cur = find_storage_device(name);
	if (!cur || !cur->blockwrite) {
		return 0;
	}

	return cur->blockwrite(cur, start_block, bytes, data);
}

/* Allocate a new file descriptor and attach it to 'file' */
int alloc_filehandle(fs_handle_type_t type, fs_directory_entry_t* file, uint32_t ibufsz, uint32_t obufsz)
{
	/* Check we havent used up all available fds */
	if (fd_alloc >= FD_MAX) {
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
	return -1;
}

/* Free a file descriptor */
uint32_t destroy_filehandle(uint32_t descriptor)
{
	dprintf("destroy_filehandle(%d)\n", descriptor);
	/* Sanity checks */
	if (descriptor >= FD_MAX || filehandles[descriptor] == NULL) {
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
		return NULL;
	}

	dprintf("fs_create_file '%s'\n", pathandfile);

	if (!verify_path(pathandfile)) {
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
		return false;
	}
	if (*pathname == 0) {
		/* A file located on the root directory -- special case */
		kfree_null(&pathname);
		pathname = strdup("/");
	}
	fs_tree_t* directory = walk_to_node(fs_tree, pathname);
	if (!directory) {
		dprintf("vfs create: no such path: %s\n", pathname);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}

	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (fileinfo) {
		dprintf("vfs create: file in %s already exists: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
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
		dprintf("vfs create dir: no such path: %s\n", pathname);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}

	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (fileinfo) {
		dprintf("vfs create dir: file in %s already exists: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	
	if (directory->responsible_driver && directory->responsible_driver->createdir) {
		uint64_t lbapos = directory->responsible_driver->createdir(directory, filename);
		/* Remove the deleted file from the fs_tree_t */
		if (lbapos) {
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
		return -1;
	}

	dprintf("_open alloc filehandle %d\n", fd);

	filehandles[fd]->cached = 0;
	if (type == file_random || type == file_input) {
		dprintf("read into buffer pos=%d sz=%d buf=%llx\n", filehandles[fd]->seekpos, file->size <= filehandles[fd]->inbufsize ? file->size : filehandles[fd]->inbufsize, filehandles[fd]->inbuf);
		/* Read an initial buffer into the structure up to fd->inbufsize in size */
		if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos,
			file->size <= filehandles[fd]->inbufsize ? file->size : filehandles[fd]->inbufsize,
			filehandles[fd]->inbuf)) {
			/* If we couldnt get the initial buffer, there is something wrong.
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
	return (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) ? (int64_t)-1 : (int64_t)filehandles[fd]->seekpos; 
}

/* Read bytes from an open file */
int _read(int fd, void *buffer, unsigned int count)
{
	/* Sanity checks */
	if (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) {
		return -1;
	}

	/* can't read from a write-only handle */
	if (filehandles[fd]->type == file_output) {
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
		dprintf("Doesnt fit buffer\n");
		int readbytes = 0;
		while (count > 0) {
			int rb;
			if (count > filehandles[fd]->inbufsize) {
				rb = filehandles[fd]->inbufsize;
			} else {
				rb = count;
			}
			dprintf("Will read %d into inbuf\n", rb);

			if (!fs_read_file(filehandles[fd]->file, filehandles[fd]->seekpos, rb, filehandles[fd]->inbuf)) {
				dprintf("fs_read_file failed in _read()\n");
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
				dprintf("fs_read_file failed in _read() [cached]\n");
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
		return -1;
	}

	/* can't truncate a read-only handle */
	if (filehandles[fd]->type == file_input) {
		return -1;
	}

	if (length > filehandles[fd]->file->size) {
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
		return -1;
	}

	/* can't write to a read-only handle */
	if (filehandles[fd]->type == file_input) {
		return -1;
	}

	dprintf("_write() fs_write_file %d\n", count);
	if (!fs_write_file(filehandles[fd]->file, filehandles[fd]->seekpos, count, buffer)) {
		return -1;
	}

	if (filehandles[fd]->seekpos >= filehandles[fd]->file->size) {
		/* Underlying driver will extend file too */
		dprintf("_write growing file from %d to %d\n", filehandles[fd]->file->size, filehandles[fd]->seekpos + count);
		filehandles[fd]->file->size = filehandles[fd]->seekpos + count;
	}

	filehandles[fd]->seekpos += count;
	dprintf("_write done\n");
	return count;
}

int _eof(int fd)
{
	return (fd < 0 || fd >= FD_MAX || filehandles[fd] == NULL) ? -1 : (filehandles[fd]->seekpos >= filehandles[fd]->file->size);
}

void retrieve_node_from_driver(fs_tree_t* node)
{
	/* XXX: Check there isnt already content in node->files, if there is,
	 * delete the old content first to avoid a memleak.
	 */

	//kprintf("retrieve_node_from_driver\n");
	if (node == NULL) {
		return;
	}

	filesystem_t* driver = (filesystem_t*)node->responsible_driver;

	if (driver == dummyfs) {
		return;
	}

	//kprintf("drv=%08x getdir=%08x\n", driver, driver->getdir);
	if (driver == NULL || driver->getdir == NULL) {
		/* Driver does not implement getdir() */
		kprintf("*** BUG *** Driver %08x on node '%s' is null or does not support getdir()! (getdir=%08x)\n", driver, node->name, driver->getdir);
		return;
	}

	//kprintf("call getdir, node->lbapos=%d node->name=%s\n", node->lbapos, node->name);

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
		return NULL;
	}

	if (current_node->dirty != 0) {
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
	if (!verify_path(path))
		return NULL;
	if (!strcmp(path, "/"))
		return fs_tree;
	/* First build the dir stack */
	dirstack_t* ds = kmalloc(sizeof(dirstack_t));
	if (!ds) {
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
	fs_tree_t* result = walk_to_node_internal(current_node, ds);
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
	if (!directory) {
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
	if (file && file->directory && file->directory->responsible_driver) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs ? fs->readfile(file, start, length, buffer) : 0;
	}
	dprintf("fs_read_file with invalid file information\n");
	return 0;
}

int fs_write_file(fs_directory_entry_t* file, uint32_t start, uint32_t length, unsigned char* buffer)
{
	if (file && file->directory && file->directory->responsible_driver) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs && fs->writefile ? fs->writefile(file, start, length, buffer) : 0;
	}
	dprintf("fs_write_file with invalid file information\n");
	return 0;
}

int fs_truncate_file(fs_directory_entry_t* file, uint32_t length)
{
	if (file && file->directory && file->directory->responsible_driver) {
		filesystem_t* fs = (filesystem_t*)file->directory->responsible_driver;
		return fs && fs->truncatefile ? fs->truncatefile(file, length) : 0;
	}
	dprintf("fs_truncate_file with invalid file information\n");
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
		kfree(temp);
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
		dprintf("vfs unlink: no such path: %s\n", pathname);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	fs_directory_entry_t* fileinfo = find_file_in_dir(directory, filename);

	if (!fileinfo) {
		dprintf("vfs unlink: no file in %s: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	
	bool rv = false;
	if (!(fileinfo->flags & FS_DIRECTORY) && directory->responsible_driver && directory->responsible_driver->rm) {
		rv = directory->responsible_driver->rm(directory, filename);
		/* Remove the deleted file from the fs_tree_t */
		if (rv) {
			delete_file_node(&(directory->files), filename);
		}
	}
	kfree_null(&pathname);
	kfree_null(&filename);
	return rv;
}

bool fs_delete_directory(const char* pathandfile)
{
	if (!verify_path(pathandfile)) {
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
		dprintf("vfs rmdir: no such path: %s\n", pathname);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	fs_directory_entry_t* fileinfo = find_dir_in_dir(directory, filename);
	if (!fileinfo) {
		dprintf("vfs rmdir: no such dir in %s: %s\n", pathname, filename);
		kfree_null(&pathname);
		kfree_null(&filename);
		return false;
	}
	
	bool rv = false;
	if ((fileinfo->flags & FS_DIRECTORY) && directory->responsible_driver && directory->responsible_driver->rmdir) {
		rv = directory->responsible_driver->rmdir(directory, filename);
		/* Remove the deleted file from the fs_tree_t */
		if (rv) {
			dprintf("Deleting from directory files %llx %llx\n", directory, directory->files);
			for (fs_directory_entry_t* f = directory->files; f; f = f->next) {
				dprintf("File: %s\n", f->filename);
			}
			delete_file_node(&(directory->files), filename);
			dprintf("Deleting from directory child dirs %llx %llx\n", directory, directory->child_dirs);
			delete_tree_node(&(directory->child_dirs), filename);
			dprintf("Deletion done\n");
		}
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
	if (item == NULL) {
		return 0;
	}
	item->responsible_driver = (void*)fs;
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
	return (fs_directory_entry_t*)(item ? item->files : NULL);
}

bool fs_is_directory(const char* pathname)
{
	fs_tree_t* item = walk_to_node(fs_tree, pathname);
	return item != NULL;
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