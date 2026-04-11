#include <udf.h>

static filesystem_t *udf_fs = NULL;
static udf_t *udf_volumes = NULL;
static char empty_string[] = "";

static udf_t *udf_find_volume(const char *device_name) {
	udf_t *walk = udf_volumes;

	for (; walk; walk = walk->next) {
		if (!strcmp(walk->device->name, device_name)) {
			return walk;
		}
	}

	return NULL;
}

static uint32_t udf_find_partition_start(udf_t *fs, uint16_t number) {
	size_t i;

	for (i = 0; i < fs->partition_count; ++i) {
		if (fs->partitions[i].number == number) {
			return fs->partitions[i].start_lba;
		}
	}

	return 0xffffffff;
}

static bool udf_read_blocks(udf_t *fs, uint32_t lba, uint32_t blocks, unsigned char *buffer) {
	return read_storage_device(fs->device->name, lba, blocks * UDF_BLOCK_SIZE, buffer);
}

static bool udf_read_partition_blocks(udf_t *fs, uint16_t part, uint32_t lba, uint32_t blocks, unsigned char *buffer) {
	uint32_t part_start = udf_find_partition_start(fs, part);

	if (part_start == 0xffffffff) {
		fs_set_error(FS_ERR_OUTSIDE_VOLUME);
		return false;
	}

	return udf_read_blocks(fs, part_start + lba, blocks, buffer);
}

static char *udf_decode_name(const unsigned char *src, size_t len)
{
	if (!src || len == 0) {
		return empty_string;
	}

	size_t outlen = src[len - 1];
	if (outlen == 0 || outlen > len - 1) {
		return empty_string;
	}

	uint8_t comp_id = src[0];
	size_t stride;
	size_t offset;
	size_t chars;

	switch (comp_id) {
		case 8: {
			stride = 1;
			offset = 0;
			chars = outlen - 1;
			break;
		}
		case 16: {
			stride = 2;
			offset = 1;
			chars = (outlen - 1) / 2;
			break;
		}
		default: {
			return empty_string;
		}
	}

	char *out = kmalloc(chars + 1);
	if (!out) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	for (size_t i = 0; i < chars; ++i) {
		out[i] = src[1 + offset + (i * stride)];
	}

	out[chars] = 0;
	return out;
}

static void udf_parse_timestamp(const unsigned char *ts, fs_directory_entry_t *entry) {
	const udf_timestamp_t *t = (const udf_timestamp_t *)ts;
	entry->year = ntohs(t->year);
	entry->month = t->month;
	entry->day = t->day;
	entry->hour = t->hour;
	entry->min = t->minute;
	entry->sec = t->second;
}

static bool udf_read_file_entry(udf_t *fs, uint16_t part, uint32_t lba, unsigned char *buffer) {
	if (!udf_read_partition_blocks(fs, part, lba, 1, buffer)) {
		return false;
	}

	const udf_descriptor_tag_t *tag = (const udf_descriptor_tag_t *)buffer;
	uint16_t id = ntohs(tag->tag_identifier);

	if (id != UDF_TAGID_FE && id != UDF_TAGID_EFE) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	return true;
}

static bool udf_read_file_data(udf_t *fs, uint16_t part, uint32_t icb_lba, uint64_t start, uint32_t length, unsigned char *buffer)
{
	unsigned char fe[UDF_BLOCK_SIZE];

	if (!udf_read_file_entry(fs, part, icb_lba, fe)) {
		return false;
	}

	const udf_descriptor_tag_t *tag = (const udf_descriptor_tag_t *)fe;
	uint16_t id = ntohs(tag->tag_identifier);

	uint64_t file_size;
	uint16_t icb_flags;
	uint32_t len_ea;
	uint32_t len_ad;
	size_t header_size;

	if (id == UDF_TAGID_FE) {
		const udf_file_entry_t *f = (const udf_file_entry_t *)fe;

		file_size = ntohll(f->information_length);
		icb_flags = ntohs(f->icb_tag.flags);
		len_ea = ntohl(f->length_of_extended_attributes);
		len_ad = ntohl(f->length_of_allocation_descriptors);
		header_size = sizeof(udf_file_entry_t);

	} else {
		const udf_extended_file_entry_t *f = (const udf_extended_file_entry_t *)fe;

		file_size = ntohll(f->information_length);
		icb_flags = ntohs(f->icb_tag.flags);
		len_ea = ntohl(f->length_of_extended_attributes);
		len_ad = ntohl(f->length_of_allocation_descriptors);
		header_size = sizeof(udf_extended_file_entry_t);
	}

	if (start >= file_size) {
		return true;
	}

	if (start + length > file_size) {
		length = file_size - start;
	}

	uint16_t ad_type = icb_flags & 0x0007;

	unsigned char *ads = fe + header_size + len_ea;

	if ((size_t)(header_size + len_ea + len_ad) > UDF_BLOCK_SIZE) {
		fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
		return false;
	}

	if (ad_type == UDF_AD_INICB) {
		if (start < len_ad) {
			uint32_t available = len_ad - start;

			if (available > length) {
				available = length;
			}

			memcpy(buffer, ads + start, available);
		}

		return true;
	}

	uint32_t done = 0;
	uint64_t file_pos = 0;
	uint32_t ad_off = 0;

	while (ad_off < len_ad && done < length) {
		uint32_t extent_length;
		uint32_t extent_lba;
		uint16_t extent_part;

		if (ad_type == UDF_AD_SHORT) {
			if (ad_off + 8 > len_ad) {
				fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
				return false;
			}

			extent_length = ntohl(*(uint32_t *)(ads + ad_off));
			extent_lba = ntohl(*(uint32_t *)(ads + ad_off + 4));
			extent_part = part;
			ad_off += 8;

		} else if (ad_type == UDF_AD_LONG) {
			if (ad_off + sizeof(udf_long_ad_t) > len_ad) {
				fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
				return false;
			}

			const udf_long_ad_t *ad = (const udf_long_ad_t *)(ads + ad_off);

			extent_length = ntohl(ad->extent_length);
			extent_lba = ntohl(ad->extent_location);
			extent_part = ntohs(ad->partition_reference_number);

			ad_off += sizeof(udf_long_ad_t);

		} else {
			fs_set_error(FS_ERR_UNSUPPORTED);
			return false;
		}

		uint32_t extent_flags = (extent_length & UDF_EXTENT_FLAG_MASK) >> 30;
		uint32_t extent_bytes = extent_length & UDF_EXTENT_LENGTH_MASK;

		if (extent_flags != UDF_EXTENT_RECORDED_ALLOCATED) {
			file_pos += extent_bytes;
			continue;
		}

		if (start >= file_pos + extent_bytes) {
			file_pos += extent_bytes;
			continue;
		}

		uint64_t local_start = 0;
		if (start > file_pos) {
			local_start = start - file_pos;
		}

		uint32_t local_length = extent_bytes - local_start;
		if (local_length > length - done) {
			local_length = length - done;
		}

		uint32_t block_offset = local_start % UDF_BLOCK_SIZE;
		uint32_t first_block = extent_lba + (local_start / UDF_BLOCK_SIZE);
		uint32_t blocks = (block_offset + local_length + UDF_BLOCK_SIZE - 1) / UDF_BLOCK_SIZE;

		unsigned char *readbuf = kmalloc(blocks * UDF_BLOCK_SIZE);
		if (!readbuf) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return false;
		}

		if (!udf_read_partition_blocks(fs, extent_part, first_block, blocks, readbuf)) {
			kfree_null(&readbuf);
			return false;
		}

		memcpy(buffer + done, readbuf + block_offset, local_length);
		add_random_entropy(*(uint64_t *)readbuf);
		kfree_null(&readbuf);

		done += local_length;
		file_pos += extent_bytes;
	}

	return true;
}

static fs_directory_entry_t *parse_directory(fs_tree_t *node, udf_t *fs, uint16_t part, uint32_t icb_lba)
{
	unsigned char fe[UDF_BLOCK_SIZE];

	if (!udf_read_file_entry(fs, part, icb_lba, fe)) {
		return NULL;
	}

	const udf_descriptor_tag_t *tag = (const udf_descriptor_tag_t *)fe;
	uint16_t id = ntohs(tag->tag_identifier);

	uint64_t dir_size;
	uint16_t icb_flags;
	uint32_t len_ea;
	uint32_t len_ad;
	size_t header_size;
	uint8_t file_type;

	if (id == UDF_TAGID_FE) {
		const udf_file_entry_t *f = (const udf_file_entry_t *)fe;

		dir_size = ntohll(f->information_length);
		icb_flags = ntohs(f->icb_tag.flags);
		len_ea = ntohl(f->length_of_extended_attributes);
		len_ad = ntohl(f->length_of_allocation_descriptors);
		header_size = sizeof(udf_file_entry_t);
		file_type = f->icb_tag.file_type;
	} else if (id == UDF_TAGID_EFE) {
		const udf_extended_file_entry_t *f = (const udf_extended_file_entry_t *)fe;

		dir_size = ntohll(f->information_length);
		icb_flags = ntohs(f->icb_tag.flags);
		len_ea = ntohl(f->length_of_extended_attributes);
		len_ad = ntohl(f->length_of_allocation_descriptors);
		header_size = sizeof(udf_extended_file_entry_t);
		file_type = f->icb_tag.file_type;
	} else {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (file_type != UDF_FT_DIRECTORY) {
		fs_set_error(FS_ERR_NOT_A_DIRECTORY);
		return NULL;
	}

	if (dir_size > MAX_REASONABLE_UDF_DIR_SIZE) {
		fs_set_error(FS_ERR_OVERSIZED_DIRECTORY);
		return NULL;
	}

	uint16_t ad_type = icb_flags & 0x0007;

	if ((size_t)(header_size + len_ea + len_ad) > UDF_BLOCK_SIZE) {
		fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
		return NULL;
	}

	unsigned char *dirdata = kmalloc(dir_size ? dir_size : 1);
	if (!dirdata) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	if (ad_type == UDF_AD_INICB) {
		memcpy(dirdata, fe + header_size + len_ea, dir_size);
	} else {
		if (!udf_read_file_data(fs, part, icb_lba, 0, dir_size, dirdata)) {
			kfree_null(&dirdata);
			return NULL;
		}
	}

	uint32_t offset = 0;
	fs_directory_entry_t *list = NULL;

	while (offset + sizeof(udf_file_identifier_descriptor_t) <= dir_size) {
		udf_file_identifier_descriptor_t *fid = (udf_file_identifier_descriptor_t *)(dirdata + offset);
		uint16_t fid_tag = ntohs(fid->descriptor_tag.tag_identifier);

		if (fid_tag != UDF_TAGID_FID) {
			break;
		}

		uint8_t file_characteristics = fid->file_characteristics;
		uint8_t name_len = fid->length_of_file_identifier;
		uint16_t impl_len = ntohs(fid->length_of_implementation_use);
		uint32_t fid_len = sizeof(udf_file_identifier_descriptor_t) + impl_len + name_len;

		fid_len = (fid_len + 3) & ~3;

		if (offset + fid_len > dir_size) {
			fs_set_error(FS_ERR_BUFFER_WOULD_OVERFLOW);
			break;
		}

		if (!(file_characteristics & UDF_FILECHAR_DELETED) && !(file_characteristics & UDF_FILECHAR_PARENT) && name_len != 0) {
			unsigned char *name_src = (unsigned char *)fid + sizeof(udf_file_identifier_descriptor_t) + impl_len;
			char *name = udf_decode_name(name_src, name_len);

			if (!name) {
				kfree_null(&dirdata);
				return NULL;
			}

			uint32_t child_lba = ntohl(fid->icb.extent_location);
			uint16_t child_part = ntohs(fid->icb.partition_reference_number);

			fs_directory_entry_t *entry = kmalloc(sizeof(fs_directory_entry_t));
			if (!entry) {
				if (name != empty_string) {
					kfree_null(&name);
				}
				kfree_null(&dirdata);
				fs_set_error(FS_ERR_OUT_OF_MEMORY);
				return NULL;
			}

			memset(entry, 0, sizeof(fs_directory_entry_t));
			entry->filename = name;
			entry->directory = node;
			entry->lbapos = UDF_PACK_ICB(child_part, child_lba);
			strlcpy(entry->device_name, fs->device->name, 16);

			unsigned char child_fe[UDF_BLOCK_SIZE];

			if (udf_read_file_entry(fs, child_part, child_lba, child_fe)) {
				const udf_descriptor_tag_t *child_tag = (const udf_descriptor_tag_t *)child_fe;
				uint16_t child_id = ntohs(child_tag->tag_identifier);

				uint64_t child_size;
				const udf_timestamp_t *child_ts;
				uint8_t child_type;

				if (child_id == UDF_TAGID_FE) {
					const udf_file_entry_t *child = (const udf_file_entry_t *)child_fe;
					child_size = ntohll(child->information_length);
					child_ts = &child->access_time;
					child_type = child->icb_tag.file_type;
				} else if (child_id == UDF_TAGID_EFE) {
					const udf_extended_file_entry_t *child = (const udf_extended_file_entry_t *)child_fe;
					child_size = ntohll(child->information_length);
					child_ts = &child->access_time;
					child_type = child->icb_tag.file_type;
				} else {
					child_size = 0;
					child_ts = NULL;
					child_type = 0;
				}

				entry->size = child_size;

				if (child_ts) {
					udf_parse_timestamp((const unsigned char *)child_ts, entry);
				}

				if (child_type == UDF_FT_DIRECTORY || (file_characteristics & UDF_FILECHAR_DIRECTORY)) {
					entry->flags |= FS_DIRECTORY;
				}
			} else {
				entry->size = 0;

				if (file_characteristics & UDF_FILECHAR_DIRECTORY) {
					entry->flags |= FS_DIRECTORY;
				}
			}

			entry->next = list;
			list = entry;
		}

		offset += fid_len;
	}

	kfree_null(&dirdata);
	return list;
}
static udf_t *udf_mount_volume(const char *name) {
	storage_device_t *dev = find_storage_device(name);
	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}

	if (dev->block_size != UDF_BLOCK_SIZE) {
		fs_set_error(FS_ERR_UNSUPPORTED);
		return NULL;
	}

	udf_t *fs = kcalloc(1, sizeof(udf_t));
	if (!fs) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	fs->device = dev;
	fs->volume_name = empty_string;

	unsigned char avdp[UDF_BLOCK_SIZE];

	if (!read_storage_device(name, UDF_AVDP_LBA, UDF_BLOCK_SIZE, avdp)) {
		kfree_null(&fs);
		return NULL;
	}

	if (ntohs(*(uint16_t *)avdp) != UDF_TAGID_AVDP) {
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs);
		return NULL;
	}

	uint32_t main_vds_len_bytes = ntohl(*(uint32_t *)(avdp + 16));
	uint32_t main_vds_lba = ntohl(*(uint32_t *)(avdp + 20));
	uint32_t main_vds_blocks = (main_vds_len_bytes + UDF_BLOCK_SIZE - 1) / UDF_BLOCK_SIZE;

	if (main_vds_blocks == 0 || main_vds_blocks > MAX_REASONABLE_UDF_VDS_BLOCKS) {
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs);
		return NULL;
	}

	unsigned char *vds = kmalloc(main_vds_blocks * UDF_BLOCK_SIZE);
	if (!vds) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		kfree_null(&fs);
		return NULL;
	}

	if (!udf_read_blocks(fs, main_vds_lba, main_vds_blocks, vds)) {
		kfree_null(&vds);
		kfree_null(&fs);
		return NULL;
	}

	uint16_t fsd_part = 0xffff;
	uint32_t fsd_lba = 0;
	uint32_t i;

	for (i = 0; i < main_vds_blocks; ++i) {
		unsigned char *desc = vds + (i * UDF_BLOCK_SIZE);
		uint16_t tag = ntohs(*(uint16_t *)desc);

		if (tag == UDF_TAGID_PD) {
			if (fs->partition_count < MAX_UDF_PARTITIONS) {
				fs->partitions[fs->partition_count].number = ntohs(*(uint16_t *)(desc + 22));
				fs->partitions[fs->partition_count].start_lba = ntohl(*(uint32_t *)(desc + 188));
				fs->partition_count++;
			}
		} else if (tag == UDF_TAGID_LVD) {
			char *volname = udf_decode_name(desc + 84, 128);

			if (volname) {
				fs->volume_name = volname;
			}

			fsd_lba = ntohl(*(uint32_t *)(desc + 248 + 4));
			fsd_part = ntohs(*(uint16_t *)(desc + 248 + 8));
		} else if (tag == UDF_TAGID_TD) {
			break;
		}
	}

	kfree_null(&vds);

	if (fs->partition_count == 0 || fsd_part == 0xffff) {
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs);
		return NULL;
	}

	unsigned char fsd[UDF_BLOCK_SIZE];

	if (!udf_read_partition_blocks(fs, fsd_part, fsd_lba, 1, fsd)) {
		kfree_null(&fs);
		return NULL;
	}

	if (ntohs(*(uint16_t *)fsd) != UDF_TAGID_FSD) {
		fs_set_error(FS_ERR_VFS_DATA);
		kfree_null(&fs);
		return NULL;
	}

	fs->root_lba = ntohl(*(uint32_t *)(fsd + 400 + 4));
	fs->root_part = ntohs(*(uint16_t *)(fsd + 400 + 8));

	return fs;
}

void *udf_get_directory(void *t) {
	fs_tree_t *tree = (fs_tree_t *)t;
	if (!tree) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	udf_t *fs = (udf_t *)tree->opaque;
	if (!fs) {
		fs_set_error(FS_ERR_VFS_DATA);
		return NULL;
	}

	if (tree->lbapos == 0) {
		return parse_directory(tree, fs, fs->root_part, fs->root_lba);
	}

	return parse_directory(tree, fs, UDF_ICB_PART(tree->lbapos), UDF_ICB_LBA(tree->lbapos));
}

bool udf_read_file(void *f, uint64_t start, uint32_t length, unsigned char *buffer) {
	fs_directory_entry_t *file = (fs_directory_entry_t *)f;
	if (!file) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	udf_t *fs = udf_find_volume(file->device_name);
	if (!fs) {
		fs_set_error(FS_ERR_VFS_DATA);
		return false;
	}

	return udf_read_file_data(fs, UDF_ICB_PART(file->lbapos), UDF_ICB_LBA(file->lbapos), start, length, buffer);
}

int udf_attach(const char *device, const char *path) {
	udf_t *vol = udf_mount_volume(device);
	if (!vol) {
		return 0;
	}

	vol->next = udf_volumes;
	udf_volumes = vol;

	return attach_filesystem(path, udf_fs, vol);
}

void init_udf() {
	udf_fs = kcalloc(1, sizeof(filesystem_t));
	if (!udf_fs) {
		return;
	}

	strlcpy(udf_fs->name, "udf", 31);
	udf_fs->mount = udf_attach;
	udf_fs->getdir = udf_get_directory;
	udf_fs->readfile = udf_read_file;
	udf_fs->writefile = NULL;
	udf_fs->truncatefile = NULL;
	udf_fs->createfile = NULL;
	udf_fs->createdir = NULL;
	udf_fs->rmdir = NULL;
	udf_fs->rm = NULL;
	udf_fs->freespace = NULL;
	register_filesystem(udf_fs);
}
