#include "../include/devfs.h"
#include "../include/filesystem.h"
#include "../include/kmalloc.h"
#include "../include/string.h"

FS_FileSystem* devfs = NULL;

void* devfs_get_directory(void* t)
{
	FS_Tree* treeitem = (FS_Tree*)t;
	return NULL;
}

int devfs_read_file(void* i, const char* filename, u32int start, u32int length, unsigned char* buffer)
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
	/* NB: The /dev mountpoint must exist in the root fs */
	attach_filesystem("/devices", devfs, NULL);
}


