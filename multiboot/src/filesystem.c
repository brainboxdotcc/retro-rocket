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
	/* XXX: Check there isnt already content in node->files, if there is,
	 * delete the old content first to avoid a memleak.
	 */
	FS_FileSystem* driver = (FS_FileSystem*)node->responsible_driver;
	if (driver->getdir == NULL)
	{
		/* Driver does not implement getdir() */
		printf("getdir not implememnted by driver!\n");
		return;
	}
	printf("driver: 0x%08x node 0x%08x %s\n", driver, node, driver->name);
	node->files = driver->getdir(node);
	node->dirty = 0;
	if (node->files == NULL)
	{
		printf("Empty directory %s!\n", node->name);
		node->child_dirs = NULL;
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
	FS_DirectoryEntry* x = (FS_DirectoryEntry*)node->files;
	for (; x->next; x = x->next)
	{
		if (x->flags & FS_DIRECTORY)
		{
			/* Insert a new child directory into node->child_dirs,
			 * Make each dir empty and 'dirty' and get its opaque
			 * from the parent dir
			 */
			if (node->child_dirs == NULL)
			{
				node->child_dirs = (FS_Tree*)kmalloc(sizeof(FS_Tree));
				node->child_dirs->next = NULL;
			}
			else
			{
				FS_Tree* newnode = (FS_Tree*)kmalloc(sizeof(FS_Tree));
				newnode->next = node->child_dirs;
				node->child_dirs = newnode;
			}

			node->child_dirs->name = x->filename;
			node->child_dirs->lbapos = x->lbapos;
			node->child_dirs->size = x->size;
			node->child_dirs->device = x->device;
			node->child_dirs->opaque = node->opaque;
			node->child_dirs->dirty = 1;
			node->child_dirs->parent = node;
			printf("REPLACEMENT OF DRIVER PTR ON %s\n", x->filename ? x->filename : "/");
			node->child_dirs->responsible_driver = node->responsible_driver;
		}
	}
}

typedef struct DirStack_t
{
	char* name;
	struct DirStack_t* next;
} DirStack;

FS_Tree* walk_to_node_internal(FS_Tree* current_node, DirStack* dir_stack)
{
	if (current_node->dirty != 0)
	{
		printf("fetch from driver\n");
		retrieve_node_from_driver(current_node);
	}

	if (current_node != NULL && !strcmp(current_node->name, dir_stack->name))
	{
		dir_stack = dir_stack->next;
		if (!dir_stack)
			return current_node;
	}

	FS_Tree* dirs = current_node->child_dirs;
	for (; dirs; dirs = dirs->next)
	{
		FS_Tree* result = walk_to_node_internal(dirs, dir_stack);
		if (result != NULL)
			return result;
	}

	return NULL;
}

FS_Tree* walk_to_node(FS_Tree* current_node, const char* path)
{
	printf("Walk to node: %s\n", path);
	if (!strcmp(path, "/"))
		return fs_tree;

	/* First build the dir stack */
	DirStack* ds = (DirStack*)kmalloc(sizeof(DirStack));
	DirStack* walk = ds;
	char* copy = strdup(path);
	char* parse;
	char* last = copy + 1;

	for (parse = copy + 1; *parse; ++parse)
	{
		if (*parse == '/')
		{
			*parse = 0;
			walk->name = strdup(last);
			last = parse + 1;

                        DirStack* next = (DirStack*)kmalloc(sizeof(DirStack));
			walk->next = next;
			next->next = 0;
			next->name = NULL;
			walk = next;
		}
	}
	walk->next = NULL;
	walk->name = strdup(last);
	FS_Tree* result = walk_to_node_internal(current_node, ds);
	for(; ds; ds = ds->next)
	{
		kfree(ds->name);
		kfree(ds);
	}
	return result;
}

int attach_filesystem(const char* virtual_path, FS_FileSystem* fs, void* opaque)
{
	FS_Tree* item = walk_to_node(fs_tree, virtual_path);
	if (item)
	{
		printf("REPLACEMENT OF DRIVER BY ATTACH\n");
		item->responsible_driver = (void*)fs;
		item->opaque = opaque;
		item->dirty = 1;
		item->files = item->child_dirs = NULL;
		retrieve_node_from_driver(item);
		printf("Driver '%s' (0x%08x) attached to vpath '%s'\n", fs->name, fs, virtual_path);
	}
	else
	{
		printf("Warning: Could not attach driver '%s' to vpath '%s'\n", fs->name, virtual_path);
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
	fs_tree->child_dirs = NULL;
	fs_tree->files = NULL;
	fs_tree->dirty = 0;
	fs_tree->opaque = NULL;

	attach_filesystem("/", filesystems, NULL);
}

FS_DirectoryEntry* fs_get_items(const char* pathname)
{
	FS_Tree* item = walk_to_node(fs_tree, pathname);
	return (FS_DirectoryEntry*)(item ? item->files : NULL);
}
