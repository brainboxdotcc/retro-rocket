#include <adfs.h>

static filesystem_t *adfs_fs = NULL;
static char empty_string[] = "";

static bool adfs_validate_map_sector(const unsigned char *sector)
{
	uint32_t sum = 0;
	for (int i = 254; i >= 0; --i) {
		sum += sector[i];

		if (sum > 0xff && i > 0) {
			sum = (sum & 0xff) + 1;
		}
	}

	return (uint8_t)sum == sector[255];
}

static bool adfs_read_sector(adfs_t *fs, uint32_t lba, unsigned char *buffer)
{
	uint32_t side = 0;
	if (lba >= 0x500) {
		side = 1;
		lba -= 0x500;
	}
	uint32_t track = lba >> 4;
	uint32_t sector = lba & 0x0f;
	uint32_t physical_sector = (track * 32) + (side * 16) + sector;
	return read_storage_device(fs->device->name, physical_sector, ADFS_SECTOR_SIZE, buffer);
}

static char *adfs_decode_name(const char *src)
{
	size_t len = 0;

	while (len < 10) {
		unsigned char c = (unsigned char)src[len] & 0x7f;

		if (c == 0 || c == '\r') {
			break;
		}

		len++;
	}

	while (len > 0 && (((unsigned char)src[len - 1] & 0x7f) == ' ')) {
		len--;
	}

	if (len == 0) {
		return empty_string;
	}

	char *out = kmalloc(len + 1);
	if (!out) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	for (size_t i = 0; i < len; ++i) {
		out[i] = (char)((unsigned char)src[i] & 0x7f);
	}

	out[len] = 0;
	return out;
}

/* ------------------------------------------------------------------------- */
/* map */

static bool adfs_build_extent_map(adfs_t *fs)
{
	unsigned char sector0[ADFS_SECTOR_SIZE];
	unsigned char sector1[ADFS_SECTOR_SIZE];

	if (!adfs_read_sector(fs, 0, sector0) || !adfs_read_sector(fs, 1, sector1)) {
		dprintf("ADFS: Failed to read map sectors\n");
		return false;
	}
	bool map0_ok = adfs_validate_map_sector(sector0);
	bool map1_ok = adfs_validate_map_sector(sector1);
	if (!map0_ok || !map1_ok) {
		fs_set_error(FS_ERR_SPACE_MAP_CHECKSUM);
		dprintf("ADFS: Free space map checksums invalid\n");
		return false;
	} else {
		dprintf("ADFS: Free space map checksum OK\n");
	}

	fs->extent_count = 1;
	fs->extents = kmalloc(sizeof(adfs_extent_t));
	if (!fs->extents) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}

	fs->extents[0].start = 2;
	fs->extents[0].length = fs->total_sectors > 2 ? fs->total_sectors - 2 : 0;

	dprintf("ADFS: Using simple extent map start=%u len=%u\n", fs->extents[0].start, fs->extents[0].length);

	return true;
}

/* ------------------------------------------------------------------------- */
/* file read */

static bool adfs_read_file_data(adfs_t *fs, uint32_t start_sector, uint64_t start, uint32_t length, unsigned char *buffer, uint32_t file_length)
{
	if (start >= file_length) {
		return true;
	}

	if (start + length > file_length) {
		length = file_length - start;
	}

	uint32_t first_sector = start_sector + (start / ADFS_SECTOR_SIZE);
	uint32_t offset = start % ADFS_SECTOR_SIZE;

	uint32_t done = 0;

	while (done < length) {
		unsigned char sector[ADFS_SECTOR_SIZE];

		if (!adfs_read_sector(fs, first_sector, sector)) {
			dprintf("ADFS: Failed reading sector %u\n", first_sector);
			return false;
		}

		uint32_t chunk = ADFS_SECTOR_SIZE - offset;
		if (chunk > (length - done)) {
			chunk = length - done;
		}

		memcpy(buffer + done, sector + offset, chunk);

		done += chunk;
		first_sector++;
		offset = 0;
	}

	return true;
}

/* ------------------------------------------------------------------------- */
/* directory */

static fs_directory_entry_t *adfs_parse_directory(fs_tree_t *node, adfs_t *fs, uint32_t dir_sector)
{
	unsigned char block[ADFS_SECTOR_SIZE * 5];

	for (uint32_t i = 0; i < 5; ++i) {
		if (!adfs_read_sector(fs, dir_sector + i, block + (i * ADFS_SECTOR_SIZE))) {
			dprintf("ADFS: Failed to read directory sector %u\n", dir_sector + i);
			return NULL;
		}
	}

	adfs_dir_header_t *hdr = (adfs_dir_header_t *)block;
	adfs_dir_tail_t *tail = (adfs_dir_tail_t *)(block + sizeof(block) - sizeof(adfs_dir_tail_t));

	if (memcmp(hdr->marker, ADFS_DIR_MARKER, 4) != 0) {
		dprintf("ADFS: Directory header missing 'Hugo' at sector %u\n", dir_sector);
		dump_hex(block, 256);
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (memcmp(tail->end_name, ADFS_DIR_MARKER, 4) != 0 &&
	    memcmp(tail->end_name, "Nick", 4) != 0) {
		dprintf("ADFS: Directory tail invalid at sector %u\n", dir_sector);
		dump_hex(tail, sizeof(*tail));
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (hdr->sequence != tail->end_master_sequence) {
		dprintf("ADFS: Directory sequence mismatch at sector %u start=%u end=%u\n",
			dir_sector, hdr->sequence, tail->end_master_sequence);
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	dprintf("ADFS: Directory header and tail OK at sector %u seq=%u\n",
		dir_sector, hdr->sequence);

	adfs_dir_entry_t *entries = (adfs_dir_entry_t *)(block + sizeof(adfs_dir_header_t));

	fs_directory_entry_t *list = NULL;

	for (uint32_t i = 0; i < ADFS_DIR_ENTRIES; ++i) {
		adfs_dir_entry_t *e = &entries[i];

		if ((e->name[0] & 0x7f) == 0) {
			continue;
		}

		char *name = adfs_decode_name(e->name);
		if (!name) {
			return NULL;
		}

		uint32_t start = ((uint32_t)e->start[0]) | ((uint32_t)e->start[1] << 8) | ((uint32_t)e->start[2] << 16);

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
		entry->lbapos = start;
		entry->size = e->length;

		strlcpy(entry->device_name, fs->device->name, 16);

		if (((unsigned char)e->name[3] & 0x80) != 0) {
			entry->flags |= FS_DIRECTORY;
		}

		dprintf("ADFS: Entry %u '%s' start=%lu len=%lu\n", i, name, entry->lbapos, entry->size);

		entry->next = list;
		list = entry;
	}

	return list;
}

/* ------------------------------------------------------------------------- */
/* mount */

static adfs_t *adfs_mount_volume(const char *name)
{
	storage_device_t *dev = find_storage_device(name);
	if (!dev) {
		dprintf("ADFS: No such storage device '%s'\n", name);
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}

	dprintf("ADFS: Mounting '%s' block_size=%u blocks=%lu\n", dev->name, dev->block_size, dev->size);

	adfs_t *fs = kcalloc(1, sizeof(adfs_t));
	if (!fs) {
		dprintf("ADFS: Out of memory allocating fs\n");
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	fs->device = dev;
	fs->total_sectors = dev->size;

	if (!adfs_build_extent_map(fs)) {
		dprintf("ADFS: Failed to read extent map for '%s'\n", name);
		kfree_null(&fs);
		return NULL;
	}

	unsigned char block[ADFS_SECTOR_SIZE * 5];

	for (uint32_t i = 0; i < 5; ++i) {
		if (!adfs_read_sector(fs, 2 + i, block + (i * ADFS_SECTOR_SIZE))) {
			dprintf("ADFS: Failed to read root directory sector %u for '%s'\n", 2 + i, name);
			kfree_null(&fs->extents);
			kfree_null(&fs);
			return NULL;
		}
	}

	adfs_dir_header_t *hdr = (adfs_dir_header_t *)block;
	adfs_dir_tail_t *tail = (adfs_dir_tail_t *)(block + sizeof(block) - sizeof(adfs_dir_tail_t));

	if (memcmp(hdr->marker, ADFS_DIR_MARKER, 4) != 0) {
		dprintf("ADFS: Root directory does not contain 'Hugo' on '%s'\n", name);
		dump_hex(block, 256);
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs->extents);
		kfree_null(&fs);
		return NULL;
	}

	if (memcmp(tail->end_name, ADFS_DIR_MARKER, 4) != 0) {
		dprintf("ADFS: Root directory tail invalid on '%s'\n", name);
		dump_hex(tail, sizeof(*tail));
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs->extents);
		kfree_null(&fs);
		return NULL;
	}

	if (hdr->sequence != tail->end_master_sequence) {
		dprintf("ADFS: Root directory sequence mismatch on '%s'\n", name);
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs->extents);
		kfree_null(&fs);
		return NULL;
	}

	dprintf("ADFS: Root directory OK on '%s' seq=%u\n", name, hdr->sequence);

	return fs;
}

/* ------------------------------------------------------------------------- */
/* VFS */

void *adfs_get_directory(void *t)
{
	fs_tree_t *tree = (fs_tree_t *)t;
	if (!tree) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	adfs_t *fs = (adfs_t *)tree->opaque;
	if (!fs) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (tree->lbapos == 0) {
		return adfs_parse_directory(tree, fs, 2);
	}

	return adfs_parse_directory(tree, fs, tree->lbapos);
}

bool adfs_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer)
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

	adfs_t *fs = (adfs_t *)file->directory->opaque;

	return adfs_read_file_data(fs, file->lbapos, start, length, buffer, file->size);
}

int adfs_attach(const char *device, const char *path)
{
	adfs_t *vol = adfs_mount_volume(device);
	if (!vol) {
		return 0;
	}
	return attach_filesystem(path, adfs_fs, vol);
}

const char *init_ramdisk_from_adfs_image(uint8_t *image, size_t length)
{
	if (image == NULL) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if ((length % ADFS_SECTOR_SIZE) != 0) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	size_t blocks = length / ADFS_SECTOR_SIZE;

	return init_ramdisk_from_memory(image, blocks, ADFS_SECTOR_SIZE);
}

void init_adfs()
{
	adfs_fs = kcalloc(1, sizeof(filesystem_t));
	if (!adfs_fs) {
		return;
	}

	strlcpy(adfs_fs->name, "adfs", 31);

	adfs_fs->mount = adfs_attach;
	adfs_fs->getdir = adfs_get_directory;
	adfs_fs->readfile = adfs_read_file;

	adfs_fs->writefile = NULL;
	adfs_fs->truncatefile = NULL;
	adfs_fs->createfile = NULL;
	adfs_fs->createdir = NULL;
	adfs_fs->rmdir = NULL;
	adfs_fs->rm = NULL;
	adfs_fs->freespace = NULL;

	register_filesystem(adfs_fs);
}