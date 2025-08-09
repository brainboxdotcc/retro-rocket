#include <kernel.h>

filesystem_t* devfs = NULL;
fs_directory_entry_t* devfs_entries = NULL;

void* devfs_get_directory(void* t)
{
	fs_tree_t* treeitem = (fs_tree_t*)t;
	devfs_entries->directory = treeitem;
	return devfs_entries;
}

bool devfs_read_file([[maybe_unused]] void* file, [[maybe_unused]] uint64_t start, [[maybe_unused]] uint32_t length, [[maybe_unused]] unsigned char* buffer)
{
	return false;
}

int devfs_attach([[maybe_unused]] const char* device, const char* path)
{
	return attach_filesystem(path, devfs, NULL);
}

void init_devfs()
{
	devfs = kmalloc(sizeof(filesystem_t));
	if (!devfs) {
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
	devfs_entries->size = 0;
	devfs_entries->flags = 0;
	devfs_entries->filename = strdup("core");
}
