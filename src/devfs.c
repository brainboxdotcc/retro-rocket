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

void init_devfs()
{
	devfs = (FS_FileSystem*)kmalloc(sizeof(FS_FileSystem));
	strlcpy(devfs->name, "devfs", 31);
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
	/* NB: The /devices mountpoint must exist in the root fs */
	attach_filesystem("/devices", devfs, NULL);
}


