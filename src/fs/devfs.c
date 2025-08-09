#include <kernel.h>

filesystem_t* devfs = NULL;
fs_directory_entry_t* devfs_entries = NULL;
fs_directory_entry_t* debug_log = NULL;

void* devfs_get_directory(void* t)
{
	fs_tree_t* treeitem = (fs_tree_t*)t;
	devfs_entries->directory = treeitem;
	return devfs_entries;
}

bool devfs_read_file([[maybe_unused]] void* file, [[maybe_unused]] uint64_t start, [[maybe_unused]] uint32_t length, [[maybe_unused]] unsigned char* buffer)
{
	fs_directory_entry_t* tree = (fs_directory_entry_t*)file;
	if (strcasecmp(tree->filename, "debug") == 0) {
		if (length == 0) {
			return true;
		}
		const char* log = dprintf_buffer_snapshot();
		const uint64_t log_length = strlen(log);
		if (length > log_length) {
			fs_set_error(FS_ERR_SEEK_PAST_END);
			kfree_null(&log);
			return false;
		}
		memcpy(buffer, log + start, length);
		kfree_null(&log);
		return true;
	}
	return false;
}

int devfs_attach([[maybe_unused]] const char* device, const char* path) {
	return attach_filesystem(path, devfs, NULL);
}

void update_sizes() {
	debug_log->size = dprintf_size();
}

void init_devfs() {
	devfs = kmalloc(sizeof(filesystem_t));
	if (!devfs) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return;
	}
	strlcpy(devfs->name, "devfs", 31);
	devfs->mount = devfs_attach;
	devfs->getdir = devfs_get_directory;
	devfs->readfile = devfs_read_file;
	devfs->writefile = NULL;
	devfs->truncatefile = NULL;
	devfs->createfile = NULL;
	devfs->createdir = NULL;
	devfs->rmdir = NULL;
	devfs->rm = NULL;
	devfs->freespace = NULL;
	register_filesystem(devfs);
	devfs_entries = kmalloc(sizeof(fs_directory_entry_t));
	if (!devfs_entries) {
		kfree_null(&devfs);
		return;
	}
	devfs_entries->next = NULL;
	devfs_entries->size = dprintf_size();
	devfs_entries->flags = 0;
	devfs_entries->filename = strdup("debug");
	debug_log = devfs_entries;

	proc_register_idle(update_sizes, IDLE_FOREGROUND);
}
