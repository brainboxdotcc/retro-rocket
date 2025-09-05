#include <kernel.h>
#include <filesystem.h>
#include <iso9660.h>

#define VERIFY_ISO9660(n) (n->standardidentifier[0] == 'C' && n->standardidentifier[1] == 'D' \
                                && n->standardidentifier[2] == '0' && n->standardidentifier[3] == '0' && n->standardidentifier[4] == '1')

fs_directory_entry_t *parse_directory(fs_tree_t *node, iso9660 *info, uint32_t start_lba, uint32_t lengthbytes);

/**
 * @brief Mount an ISO 9660 filesystem on a given block device name.
 *
 * Returns either NULL or an iso9660* which references the volume information and initially the
 * root directory of the disk.
 * 
 * @param device device name
 * @return iso9660* detail of root directory, or null on error
 */
iso9660 *iso_mount_volume(const char *device);

bool iso_read_file(void *file, uint64_t start, uint32_t length, unsigned char *buffer);

static filesystem_t *iso9660_fs = NULL;

void parse_boot([[maybe_unused]] iso9660 *info, [[maybe_unused]] unsigned char *buffer) {
}

int parse_pvd(iso9660 *info, unsigned char *buffer) {
	PVD *pvd = (PVD *) buffer;
	if (!VERIFY_ISO9660(pvd)) {
		fs_set_error(FS_ERR_INVALID_PVD);
		return 0;
	}
	char *ptr = pvd->volumeidentifier + 31;
	for (; ptr != pvd->volumeidentifier && *ptr == ' '; --ptr) {
		// Backpeddle over the trailing spaces in the volume name
		if (*ptr == ' ')
			*ptr = 0;
	}

	size_t j = 0;
	info->volume_name = kmalloc(strlen(pvd->volumeidentifier) + 1);
	if (!info->volume_name) {
		return false;
	}
	for (ptr = pvd->volumeidentifier; *ptr && j < strlen(pvd->volumeidentifier); ++ptr) {
		info->volume_name[j++] = *ptr;
	}
	// Null-terminate volume name
	info->volume_name[j] = 0;

	info->joliet = 0;
	info->pathtable_lba = pvd->lsb_pathtable_L_lba;
	info->rootextent_lba = pvd->root_directory.extent_lba_lsb;
	info->rootextent_len = pvd->root_directory.data_length_lsb;
	info->root = parse_directory(NULL, info, pvd->root_directory.extent_lba_lsb, pvd->root_directory.data_length_lsb);

	return info->root != 0;
}

#define MAX_REASONABLE_ISO_DIR_SIZE 1024 * 1024

fs_directory_entry_t *parse_directory(fs_tree_t *node, iso9660 *info, uint32_t start_lba, uint32_t lengthbytes) {

	if (lengthbytes < 2048) {
		lengthbytes *= 2048;
	}

	unsigned char dirbuffer[lengthbytes + 1];
	memset(dirbuffer, 0, lengthbytes);

	if (!read_storage_device(info->device->name, start_lba, lengthbytes, dirbuffer)) {
		return NULL;
	}

	uint32_t dir_items = 0;
	fs_directory_entry_t *list = NULL;

	unsigned char *walkbuffer = dirbuffer;
	int entrycount = 0;

	if (lengthbytes > MAX_REASONABLE_ISO_DIR_SIZE) {
		fs_set_error(FS_ERR_OVERSIZED_DIRECTORY);
		return NULL;
	}

	while (walkbuffer < dirbuffer + lengthbytes) {

		ISO9660_directory *fentry = (ISO9660_directory *) walkbuffer;

		if (fentry->length == 0) {
			break;
		}

		// Sanity: Does this entry stay within the buffer?
		if (walkbuffer + fentry->length > dirbuffer + lengthbytes) {
			fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
			break;
		}

		++entrycount;

		// Only process past . and ..
		if (entrycount > 2) {
			fs_directory_entry_t *thisentry = kmalloc(sizeof(fs_directory_entry_t));
			if (!thisentry) {
				fs_set_error(FS_ERR_OUT_OF_MEMORY);
				return NULL;
			}

			// Calculate safe max filename length within this entry
			uint8_t safe_filename_length = fentry->filename_length;

			// The file identifier always starts at offset 33
			uint8_t *fname_start = (uint8_t *) fentry + 33;

			if (fname_start + safe_filename_length > walkbuffer + fentry->length) {
				// Clamp filename length to stay within record
				safe_filename_length = (walkbuffer + fentry->length) - fname_start;
			}

			if (safe_filename_length == 0) {
				kfree_null(&thisentry);
				walkbuffer += fentry->length;
				continue;
			}

			if (info->joliet == 0) {
				uint32_t safe_len = fentry->filename_length;
				thisentry->filename = kmalloc(safe_len + 1);
				if (!thisentry->filename) {
					fs_set_error(FS_ERR_OUT_OF_MEMORY);
					return NULL;
				}
				uint32_t j = 0;
				char *ptr = fentry->filename;

				for (; j < safe_len; ++ptr) {
					if (*ptr == ';') break;
					thisentry->filename[j++] = tolower(*ptr);
				}

				thisentry->filename[j] = 0;

				if (j > 0 && thisentry->filename[j - 1] == '.')
					thisentry->filename[j - 1] = 0;

			} else {
				uint32_t safe_len = fentry->filename_length / 2;
				thisentry->filename = kmalloc(safe_len + 1);
				if (!thisentry->filename) {
					fs_set_error(FS_ERR_OUT_OF_MEMORY);
					return NULL;
				}
				uint32_t j = 0;
				char *ptr = fentry->filename;

				for (; j < safe_len; ptr += 2) {
					if (*ptr != 0) {
						thisentry->filename[j++] = '?';
					} else {
						thisentry->filename[j++] = *(ptr + 1);
					}
				}

				thisentry->filename[j] = 0;
			}

			thisentry->year = fentry->recording_date.years_since_1900 + 1900;
			thisentry->month = fentry->recording_date.month;
			thisentry->day = fentry->recording_date.day;
			thisentry->hour = fentry->recording_date.hour;
			thisentry->min = fentry->recording_date.minute;
			thisentry->sec = fentry->recording_date.second;
			strlcpy(thisentry->device_name, info->device->name, 16);
			thisentry->device = 0;
			thisentry->lbapos = fentry->extent_lba_lsb;
			thisentry->size = fentry->data_length_lsb;
			thisentry->flags = 0;
			thisentry->directory = node;

			if (fentry->file_flags & 0x02) {
				thisentry->flags |= FS_DIRECTORY;
			}

			thisentry->next = list;
			list = thisentry;

			++dir_items;
		}

		walkbuffer += fentry->length;
	}
	return list;
}

int parse_svd(iso9660 *info, unsigned char *buffer) {
	PVD *svd = (PVD *) buffer;
	if (!VERIFY_ISO9660(svd)) {
		fs_set_error(FS_ERR_INVALID_SVD);
		return 0;
	}

	int joliet = 0;
	if (svd->escape_seq[0] == '%' && svd->escape_seq[1] == '/') {
		switch (svd->escape_seq[2]) {
			case '@':
				joliet = 1;
				break;
			case 'C':
				joliet = 2;
				break;
			case 'E':
				joliet = 3;
				break;
		}
	}

	if (joliet) {
		kprintf("Joliet extensions found on CD drive %s, UCS-2 Level %d\n", info->device->name, joliet);
		info->joliet = joliet;
		info->pathtable_lba = svd->lsb_pathtable_L_lba;
		info->rootextent_lba = svd->root_directory.extent_lba_lsb;
		info->rootextent_len = svd->root_directory.data_length_lsb;
		info->root = parse_directory(NULL, info, svd->root_directory.extent_lba_lsb, svd->root_directory.data_length_lsb);
	}
	return 1;
}

void parse_vpd([[maybe_unused]] iso9660 *info, [[maybe_unused]] unsigned char *buffer) {
}

fs_directory_entry_t *hunt_entry(iso9660 *info, const char *filename, uint32_t flags) {
	fs_directory_entry_t *currententry = info->root;
	for (; currententry->next; currententry = currententry->next) {
		if ((flags != 0) && !(currententry->flags & flags)) {
			continue;
		}
		if (!strcmp(filename, currententry->filename)) {
			return currententry;
		}
	}
	return NULL;
}

void *iso_get_directory(void *t) {
	fs_tree_t *treeitem = (fs_tree_t *) t;
	if (treeitem) {
		iso9660 *info = (iso9660 *) treeitem->opaque;
		/* The root node has no size except rotextent_len, we can get it from the vfs tree */
		uint32_t size = info->rootextent_len;
		if (treeitem->lbapos && treeitem->lbapos != info->rootextent_lba) {
			size = treeitem->size;
		}
		return parse_directory(treeitem, (iso9660 *) treeitem->opaque, treeitem->lbapos ? treeitem->lbapos : info->rootextent_lba, size);
	} else {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}
}

bool iso_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer) {
	fs_directory_entry_t *file = (fs_directory_entry_t *) f;
	storage_device_t *fs = find_storage_device(file->device_name);

	if (!fs) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	uint64_t sectors_size = length / fs->block_size;
	uint64_t sectors_start = start / fs->block_size + file->lbapos;

	// Because its not valid to read 0 sectors, we must make sure we read at least one,
	// and to make sure we read the remainder because this is an integer division,
	// we must read one more than we asked for for safety.
	sectors_size++;

	unsigned char *readbuf = kmalloc(sectors_size * fs->block_size);
	if (!readbuf) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return false;
	}
	if (!read_storage_device(file->device_name, sectors_start, length, readbuf)) {
		kfree_null(&readbuf);
		return false;
	}
	memcpy(buffer, readbuf + (start % fs->block_size), length);

	add_random_entropy(*(uint64_t *) readbuf);

	kfree_null(&readbuf);
	return true;
}

iso9660 *iso_mount_volume(const char *name) {
	storage_device_t *fs = find_storage_device(name);
	if (!fs) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}

	unsigned char *buffer = kmalloc(fs->block_size);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	iso9660 *info = kmalloc(sizeof(iso9660));
	if (!info) {
		kfree_null(&buffer);
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	memset(info, 0, sizeof(iso9660));
	memset(buffer, 0, fs->block_size);
	uint32_t volume_descriptor_offset = PVD_LBA;
	int found_pvd = 0, found_svd = 0;
	info->device = fs;
	while (1) {
		if (!read_storage_device(name, volume_descriptor_offset++, fs->block_size, buffer)) {
			kfree_null(&info);
			kfree_null(&buffer);
			return NULL;
		}
		unsigned char VolumeDescriptorID = buffer[0];
		if (VolumeDescriptorID == 0xFF) {
			// Volume descriptor terminator
			break;
		} else if (VolumeDescriptorID == 0x00) {
			parse_boot(info, buffer);
		} else if (VolumeDescriptorID == 0x01 && !found_pvd) {
			// Primary volume descriptor
			if (!parse_pvd(info, buffer)) {
				kfree_null(&info);
				kfree_null(&buffer);
				return NULL;
			}
			found_pvd = 1;
		} else if (VolumeDescriptorID == 0x02 && !found_svd) {
			// Supplementary volume descriptor
			if (!parse_svd(info, buffer)) {
				kfree_null(&info);
				kfree_null(&buffer);
				return NULL;
			}
			found_svd = 1;
		} else if (VolumeDescriptorID == 0x03) {
			// Volume partition descriptor
			parse_vpd(info, buffer);
		}
	}

	kfree_null(&buffer);
	return info;
}

int iso9660_attach(const char *device, const char *path) {
	iso9660 *vol = iso_mount_volume(device);
	if (!vol) {
		return 0;
	}
	return attach_filesystem(path, iso9660_fs, vol);
}

void init_iso9660() {
	iso9660_fs = kmalloc(sizeof(filesystem_t));
	if (!iso9660_fs) {
		return;
	}
	strlcpy(iso9660_fs->name, "iso9660", 31);
	iso9660_fs->mount = iso9660_attach;
	iso9660_fs->getdir = iso_get_directory;
	iso9660_fs->readfile = iso_read_file;
	iso9660_fs->writefile = NULL;
	iso9660_fs->truncatefile = NULL;
	iso9660_fs->createfile = NULL;
	iso9660_fs->createdir = NULL;
	iso9660_fs->rmdir = NULL;
	iso9660_fs->rm = NULL;
	iso9660_fs->freespace = NULL;
	register_filesystem(iso9660_fs);
}
