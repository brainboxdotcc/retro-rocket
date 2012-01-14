#include "../include/devfs.h"
#include "../include/filesystem.h"
#include "../include/kmalloc.h"
#include "../include/string.h"
#include "../include/kprintf.h"

FS_FileSystem* devfs = NULL;
FS_DirectoryEntry* devfs_entries = NULL;

void* devfs_get_directory(void* t)
{
	FS_Tree* treeitem = (FS_Tree*)t;
	devfs_entries->directory = treeitem;
	return devfs_entries;
}

int devfs_read_file(void* file, u32int start, u32int length, unsigned char* buffer)
{
	return 0;
}

void init_devfs()
{
	return;
	devfs = (FS_FileSystem*)kmalloc(sizeof(FS_FileSystem));
	strlcpy(devfs->name, "devfs", 31);
	devfs->getdir = devfs_get_directory;
	devfs->readfile = devfs_read_file;
	devfs->writefile = NULL;
	devfs->rm = NULL;
	register_filesystem(devfs);
	FS_DirectoryEntry* empty = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
	empty->next = NULL;
	empty->flags = 0;
	empty->filename = NULL;
	devfs_entries = (FS_DirectoryEntry*)kmalloc(sizeof(FS_DirectoryEntry));
	devfs_entries->next = empty;
	devfs_entries->size = 0;
	devfs_entries->flags = 0;
	devfs_entries->filename = strdup("core");
	//kprintf("Calling attach\n");
	/* NB: The /devices mountpoint must exist in the root fs */
	attach_filesystem("/devices", devfs, NULL);
	//kprintf("attach done\n");
}


