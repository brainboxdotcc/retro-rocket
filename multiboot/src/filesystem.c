#include "../include/filesystem.h"
#include "../include/kmalloc.h"
#include "../include/string.h"
#include "../include/printf.h"

FS_FileSystem* filesystems;
FS_Tree* fs_tree;

int register_filesystem(FS_FileSystem* newfs)
{
	printf("Registering filesystem '%s'\n", newfs->name);
	/* Add the new filesystem to the start of the list */
	newfs->next = filesystems;
	filesystems = newfs;
	return 1;
}

void retrieve_node_from_driver(FS_Tree* node)
{
	/* XXX: Check there isnt already content in node->files */
	FS_FileSystem* driver = node->responsible_driver;
	node->files = driver->getdir(node);
	node->dirty = 0;
	FS_DirectoryEntry* x = node->files;
	for (; x; x = x->next)
	{
		if (x->flags & FS_DIRECTORY)
		{
		}
	}
}

FS_Tree* walk_to_node(FS_Tree* current_node, const char* virtual_path)
{
	/* Request for root dir always works */
	if (!strcmp(virtual_path, "/"))
		return fs_tree;

	char* copy = strdup(virtual_path);
	char* findslash;
	for (findslash = copy; *findslash != 0 && *findslash != '/'; ++findslash);
	*findslash = '0';

	if (current_node->dirty != 0)
		retrieve_node_from_driver(current_node);

	if (current_node->name != NULL && !strcmp(current_node->name, copy))
	{
		kfree(copy);
		return current_node;
	}

	u32int index = 0;
	for (index = 0; index < current_node->num_child_dirs; index++)
	{
		FS_Tree* result = walk_to_node(current_node->child_dirs[index++], virtual_path);
		if (result != NULL)
		{
			kfree(copy);
			return result;
		}
	}
	return NULL;
}

int attach_filesystem(const char* virtual_path, FS_FileSystem* fs)
{
	FS_Tree* item = walk_to_node(fs_tree, virtual_path);
	if (item)
	{
		item->responsible_driver = fs;
		item->opaque = NULL;
		retrieve_node_from_driver(item);
	}
	else
	{
		printf("Warning: Could not attach filesystem '%s' to vpath '%s'\n", fs->name, virtual_path);
	}
	return 1;
}

void init_filesystem()
{
	filesystems = (FS_FileSystem*)kmalloc(sizeof(FS_FileSystem));
	strlcpy(filesystems->name, "DummyFS", 31);
	filesystems->getdir = NULL;
	filesystems->readfile = NULL;
	filesystems->writefile = NULL;
	filesystems->rm = NULL;
	filesystems->next = NULL;

	fs_tree = (FS_Tree*)kmalloc(sizeof(FS_Tree));
	fs_tree->name = NULL;
	fs_tree->parent = fs_tree;
	fs_tree->num_child_dirs = 0;
	fs_tree->child_dirs = NULL;
	fs_tree->files = NULL;
	fs_tree->dirty = 0;
	fs_tree->responsible_driver = filesystems; /* DummyFS */
	fs_tree->opaque = NULL;
}
