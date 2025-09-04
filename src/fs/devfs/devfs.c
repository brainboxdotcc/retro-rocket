#include <kernel.h>

static filesystem_t *devfs = NULL;
static devfs_node_t *devfs_head = NULL;

static void *devfs_get_directory([[maybe_unused]] void *t) {
	fs_tree_t *treeitem = (fs_tree_t *)t;

	/* Wire correct directory pointer for each entry */
	devfs_node_t *n = devfs_head;
	while (n) {
		n->ent.directory = treeitem;
		n = n->next;
	}

	return devfs_head ? &devfs_head->ent : NULL;
}

static bool devfs_read_file(void *file, uint64_t start, uint32_t length, unsigned char *buffer) {
	fs_directory_entry_t *ent = (fs_directory_entry_t *)file;
	devfs_node_t *n = (devfs_node_t *)ent; /* safe: ent is first member */

	if (!n->read_cb) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}
	return n->read_cb(start, length, buffer);
}

static int devfs_attach([[maybe_unused]] const char *device, const char *path) {
	return attach_filesystem(path, devfs, NULL);
}

void devfs_register_text(const char *name, devfs_update_cb_t update_cb, devfs_read_cb_t read_cb) {
	devfs_node_t *node = kmalloc(sizeof(devfs_node_t));
	if (!node) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return;
	}
	memset(node, 0, sizeof(*node));

	node->ent.filename = strdup(name);
	node->ent.size = 0;
	node->ent.flags = 0;
	node->update_cb = update_cb;
	node->read_cb = read_cb;

	/* Insert at head - safe, we don't mutate older nodes' .next later */
	node->next = devfs_head;
	devfs_head = node;
}

static void devfs_update_sizes(void) {
	devfs_node_t *n = devfs_head;
	while (n) {
		if (n->update_cb) {
			n->update_cb(&n->ent);
		}
		n = n->next;
	}
}

void init_devfs(void) {
	devfs = kmalloc(sizeof(filesystem_t));
	if (!devfs) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return;
	}

	strlcpy(devfs->name, "devfs", sizeof(devfs->name));
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

	/* Built-in /devices/debug device */
	init_debuglog();

	/* Periodically update sizes */
	proc_register_idle(devfs_update_sizes, IDLE_FOREGROUND, 100);
}
