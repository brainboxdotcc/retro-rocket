#include <dfs.h>

static filesystem_t *dfs_fs = NULL;
static char empty_string[] = "";

static bool dfs_read_sector(dfs_t *fs, uint32_t side, uint32_t lba, unsigned char *buffer)
{
	if (!fs || !buffer) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	if (lba >= fs->sectors_per_side) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	if (!fs->is_dsd) {
		return read_storage_device(fs->device->name, lba, DFS_SECTOR_SIZE, buffer);
	}

	if (side > 1) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	uint32_t track = lba / DFS_SECTORS_PER_TRACK;
	uint32_t sector = lba % DFS_SECTORS_PER_TRACK;
	uint32_t physical = (track * DFS_SECTORS_PER_TRACK * 2) + (side * DFS_SECTORS_PER_TRACK) + sector;

	return read_storage_device(fs->device->name, physical, DFS_SECTOR_SIZE, buffer);
}

static uint32_t dfs_catalogue_entry_count(const dfs_catalogue_sector1_t *cat1)
{
	uint32_t count = cat1->entry_count_x8 / 8;

	if (count > DFS_MAX_FILES) {
		count = DFS_MAX_FILES;
	}

	return count;
}

static uint32_t dfs_catalogue_sector_count(const dfs_catalogue_sector1_t *cat1)
{
	return ((uint32_t)(cat1->sector_hi & 0x03) << 8) | cat1->sector_lo;
}

static uint32_t dfs_file_length(const dfs_catalogue_meta_t *meta)
{
	return ((uint32_t)(meta->hi_bits & 0x30) << 12) | ((uint32_t)meta->length_mid << 8) | meta->length_lo;
}

static uint32_t dfs_file_start(const dfs_catalogue_meta_t *meta)
{
	return ((uint32_t)(meta->hi_bits & 0x03) << 8) | meta->start_lo;
}

static bool dfs_side_is_formatted(dfs_t *fs, uint32_t side)
{
	unsigned char sector0[DFS_SECTOR_SIZE];
	unsigned char sector1[DFS_SECTOR_SIZE];

	if (!dfs_read_sector(fs, side, 0, sector0) || !dfs_read_sector(fs, side, 1, sector1)) {
		return false;
	}

	dfs_catalogue_sector1_t *cat1 = (dfs_catalogue_sector1_t *)sector1;
	uint32_t entry_count = dfs_catalogue_entry_count(cat1);
	uint32_t sector_count = dfs_catalogue_sector_count(cat1);

	if (entry_count > DFS_MAX_FILES) {
		return false;
	}

	if (sector_count == 0 || sector_count > fs->sectors_per_side) {
		return false;
	}

	return true;
}

static char *dfs_decode_name(const dfs_catalogue_entry_t *entry)
{
	char buf[16];
	size_t len = 0;
	unsigned char dir = (unsigned char)entry->directory & 0x7f;

	if (dir != '$' && dir != ' ') {
		buf[len++] = (char)dir;
		buf[len++] = '.';
	}

	for (int i = 0; i < 7; ++i) {
		unsigned char c = (unsigned char)entry->name[i] & 0x7f;

		if (c == 0 || c == ' ') {
			break;
		}

		buf[len++] = (char)c;
	}

	if (len == 0) {
		return empty_string;
	}

	buf[len] = 0;

	char *out = strdup(buf);
	if (!out) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	return out;
}

static fs_directory_entry_t *dfs_make_directory_entry(fs_tree_t *node, dfs_t *fs, const char *name, uint32_t lbapos, fs_directory_entry_t *list)
{
	fs_directory_entry_t *entry = kcalloc(1, sizeof(fs_directory_entry_t));
	if (!entry) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	entry->filename = strdup(name);
	if (!entry->filename) {
		kfree_null(&entry);
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	entry->flags = FS_DIRECTORY;
	entry->directory = node;
	entry->lbapos = lbapos;
	strlcpy(entry->device_name, fs->device->name, 16);

	entry->next = list;
	return entry;
}

static fs_directory_entry_t *dfs_parse_side(fs_tree_t *node, dfs_t *fs, uint32_t side)
{
	unsigned char sector0[DFS_SECTOR_SIZE];
	unsigned char sector1[DFS_SECTOR_SIZE];

	if (!dfs_read_sector(fs, side, 0, sector0) || !dfs_read_sector(fs, side, 1, sector1)) {
		return NULL;
	}

	dfs_catalogue_sector0_t *cat0 = (dfs_catalogue_sector0_t *)sector0;
	dfs_catalogue_sector1_t *cat1 = (dfs_catalogue_sector1_t *)sector1;

	uint32_t entry_count = dfs_catalogue_entry_count(cat1);
	uint32_t sector_count = dfs_catalogue_sector_count(cat1);

	if (sector_count == 0 || sector_count > fs->sectors_per_side) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	fs_directory_entry_t *list = NULL;

	for (uint32_t i = 0; i < entry_count; ++i) {
		dfs_catalogue_entry_t *cat_entry = &cat0->entries[i];
		dfs_catalogue_meta_t *cat_meta = &cat1->meta[i];

		if (((unsigned char)cat_entry->name[0] & 0x7f) == 0) {
			continue;
		}

		char *name = dfs_decode_name(cat_entry);
		if (!name) {
			return NULL;
		}

		uint32_t start_sector = dfs_file_start(cat_meta);
		uint32_t file_length = dfs_file_length(cat_meta);

		if (start_sector >= sector_count) {
			if (name != empty_string) {
				kfree_null(&name);
			}
			continue;
		}

		uint32_t sectors_used = (file_length + DFS_SECTOR_SIZE - 1) / DFS_SECTOR_SIZE;
		if (start_sector + sectors_used > sector_count) {
			if (name != empty_string) {
				kfree_null(&name);
			}
			continue;
		}

		fs_directory_entry_t *entry = kcalloc(1, sizeof(fs_directory_entry_t));
		if (!entry) {
			if (name != empty_string) {
				kfree_null(&name);
			}
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}

		entry->filename = name;
		entry->directory = node;
		entry->size = file_length;
		entry->lbapos = side == 1 ? (DFS_LBAPOS_SIDE_FLAG | start_sector) : start_sector;

		strlcpy(entry->device_name, fs->device->name, 16);

		entry->next = list;
		list = entry;
	}

	return list;
}

static fs_directory_entry_t *dfs_root_listing(fs_tree_t *node, dfs_t *fs)
{
	if (!fs->is_dsd) {
		return dfs_parse_side(node, fs, 0);
	}

	fs_directory_entry_t *list = NULL;

	if (fs->side_valid[0]) {
		list = dfs_make_directory_entry(node, fs, "side1", DFS_LBAPOS_SIDE1_DIR, list);
		if (!list) {
			return NULL;
		}
	}

	if (fs->side_valid[1]) {
		list = dfs_make_directory_entry(node, fs, "side2", DFS_LBAPOS_SIDE2_DIR, list);
		if (!list) {
			return NULL;
		}
	}

	return list;
}

static dfs_t *dfs_mount_volume(const char *name)
{
	storage_device_t *dev = find_storage_device(name);
	if (!dev) {
		dprintf("DFS: No such storage device '%s'\n", name);
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}

	if (dev->block_size != DFS_SECTOR_SIZE) {
		dprintf("DFS: Unsupported block size %u on '%s'\n", dev->block_size, dev->name);
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	dfs_t *fs = kcalloc(1, sizeof(dfs_t));
	if (!fs) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	fs->device = dev;

	if (dev->size == DFS_IMAGE_SSD_SECTORS) {
		fs->is_dsd = false;
		fs->sectors_per_side = DFS_IMAGE_SSD_SECTORS;
	} else if (dev->size == DFS_IMAGE_DSD_SECTORS) {
		fs->is_dsd = true;
		fs->sectors_per_side = DFS_IMAGE_DSD_SECTORS / 2;
	} else {
		dprintf("DFS: Unsupported image size %lu sectors on '%s'\n", dev->size, dev->name);
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs);
		return NULL;
	}

	fs->side_valid[0] = dfs_side_is_formatted(fs, 0);
	fs->side_valid[1] = fs->is_dsd ? dfs_side_is_formatted(fs, 1) : false;

	if (!fs->is_dsd) {
		if (!fs->side_valid[0]) {
			dprintf("DFS: SSD image '%s' is not formatted\n", dev->name);
			fs_set_error(FS_ERR_VFS_DATA);
			kfree_null(&fs);
			return NULL;
		}
	} else {
		if (!fs->side_valid[0] && !fs->side_valid[1]) {
			dprintf("DFS: DSD image '%s' has no formatted sides\n", dev->name);
			fs_set_error(FS_ERR_VFS_DATA);
			kfree_null(&fs);
			return NULL;
		}
	}

	dprintf("DFS: Mounted '%s' as %s side1=%d side2=%d\n",
		dev->name,
		fs->is_dsd ? "DSD" : "SSD",
		fs->side_valid[0],
		fs->side_valid[1]);

	return fs;
}

void *dfs_get_directory(void *t)
{
	fs_tree_t *tree = (fs_tree_t *)t;
	if (!tree) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	dfs_t *fs = (dfs_t *)tree->opaque;
	if (!fs) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (!fs->is_dsd) {
		return dfs_parse_side(tree, fs, 0);
	}

	if (tree->lbapos == DFS_LBAPOS_SIDE1_DIR) {
		return dfs_parse_side(tree, fs, 0);
	}

	if (tree->lbapos == DFS_LBAPOS_SIDE2_DIR) {
		return dfs_parse_side(tree, fs, 1);
	}

	return dfs_root_listing(tree, fs);
}

bool dfs_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer)
{
	fs_directory_entry_t *file = (fs_directory_entry_t *)f;
	if (!file) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	if (!file->directory || !file->directory->opaque) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	dfs_t *fs = (dfs_t *)file->directory->opaque;
	uint32_t side = (file->lbapos & DFS_LBAPOS_SIDE_FLAG) ? 1 : 0;
	uint32_t start_sector = file->lbapos & ~DFS_LBAPOS_SIDE_FLAG;

	if (start >= file->size) {
		return true;
	}

	if (start + length > file->size) {
		length = file->size - start;
	}

	uint32_t sector = start_sector + (start / DFS_SECTOR_SIZE);
	uint32_t offset = start % DFS_SECTOR_SIZE;
	uint32_t done = 0;

	while (done < length) {
		unsigned char tmp[DFS_SECTOR_SIZE];

		if (!dfs_read_sector(fs, side, sector, tmp)) {
			dprintf("DFS: Failed reading side %u sector %u\n", side, sector);
			return false;
		}

		uint32_t chunk = DFS_SECTOR_SIZE - offset;
		if (chunk > (length - done)) {
			chunk = length - done;
		}

		memcpy(buffer + done, tmp + offset, chunk);

		done += chunk;
		sector++;
		offset = 0;
	}

	return true;
}

int dfs_attach(const char *device, const char *path)
{
	dfs_t *vol = dfs_mount_volume(device);
	if (!vol) {
		return 0;
	}

	return attach_filesystem(path, dfs_fs, vol);
}

const char *init_ramdisk_from_dfs_image(uint8_t *image, size_t length)
{
	if (!image) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if ((length % DFS_SECTOR_SIZE) != 0) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	size_t blocks = length / DFS_SECTOR_SIZE;

	if (blocks != DFS_IMAGE_SSD_SECTORS && blocks != DFS_IMAGE_DSD_SECTORS) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	return init_ramdisk_from_memory(image, blocks, DFS_SECTOR_SIZE);
}

void init_dfs(void)
{
	dfs_fs = kcalloc(1, sizeof(filesystem_t));
	if (!dfs_fs) {
		return;
	}

	strlcpy(dfs_fs->name, "dfs", 31);

	dfs_fs->mount = dfs_attach;
	dfs_fs->getdir = dfs_get_directory;
	dfs_fs->readfile = dfs_read_file;

	dfs_fs->writefile = NULL;
	dfs_fs->truncatefile = NULL;
	dfs_fs->createfile = NULL;
	dfs_fs->createdir = NULL;
	dfs_fs->rmdir = NULL;
	dfs_fs->rm = NULL;
	dfs_fs->freespace = NULL;

	register_filesystem(dfs_fs);
}
