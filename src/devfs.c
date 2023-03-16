#include <kernel.h>

FS_FileSystem* devfs = NULL;
FS_DirectoryEntry* devfs_entries = NULL;

void* devfs_get_directory(void* t)
{
	FS_Tree* treeitem = (FS_Tree*)t;
	devfs_entries->directory = treeitem;
	return devfs_entries;
}

int devfs_read_file(void* file, uint32_t start, uint32_t length, unsigned char* buffer)
{
	return 0;
}

int devfs_attach(const char* device, const char* path)
{
	return attach_filesystem(path, devfs, NULL);
}

void init_devfs()
{
	devfs = (FS_FileSystem*)kmalloc(sizeof(FS_FileSystem));
	strlcpy(devfs->name, "devfs", 31);
	devfs->mount = devfs_attach;
	devfs->getdir = devfs_get_directory;
	devfs->readfile = devfs_read_file;
	devfs->writefile = NULL;
	devfs->rm = NULL;
	register_filesystem(devfs);
	devfs_entries = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
	devfs_entries->next = NULL;
	devfs_entries->size = 0;
	devfs_entries->flags = 0;
	devfs_entries->filename = strdup("core");
}
