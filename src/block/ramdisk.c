#include "kernel.h"
#include <zlib/zlib.h>

typedef struct ramdisk_t {
	const char *name;
	uint8_t *data;
	size_t blocks;
	size_t blocksize;
} ramdisk_t;

static struct hashmap *disks = NULL;

int rd_compare(const void *a, const void *b,[[maybe_unused]

]
void *udata
) {
const ramdisk_t *rd1 = a;
const ramdisk_t *rd2 = b;
return
strcmp(rd1
->name, rd2->name);
}

uint64_t rd_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const ramdisk_t *rd = item;
	return hashmap_sip(rd->name, strlen(rd->name), seed0, seed1);
}

int rd_block_read(void *dev, uint64_t start, uint32_t bytes, unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev;
	if (!sd) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}
	ramdisk_t find_disk = {.name = sd->name};
	ramdisk_t *disk = hashmap_get(disks, &find_disk);
	if (disk) {
		uint32_t divided_length = bytes / sd->block_size;
		while (divided_length * sd->block_size < bytes) {
			divided_length++;
		}
		if (divided_length < 1) {
			divided_length++;
		}
		if (start + divided_length > disk->blocks) {
			dprintf("Requested sector %lx plus len %x is greater than ramdisk size of %lx sectors\n", start, divided_length, disk->blocks);
			return 0;
		}
		divided_length *= sd->block_size;
		memcpy(buffer, disk->data + (start * sd->block_size), divided_length);
		return 1;
	}
	return 0;
}

int rd_block_write(void *dev, uint64_t start, uint32_t bytes, const unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev;
	if (!sd) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}
	ramdisk_t find_disk = {.name = sd->name};
	ramdisk_t *disk = hashmap_get(disks, &find_disk);
	if (disk) {
		uint32_t divided_length = bytes / sd->block_size;
		while (divided_length * sd->block_size < bytes) {
			divided_length++;
		}
		if (divided_length < 1) {
			divided_length++;
		}
		if (start + divided_length > disk->blocks) {
			dprintf("Requested sector %lx plus len %x is greater than ramdisk size of %lx sectors\n", start, divided_length, disk->blocks);
			return 0;
		}
		divided_length *= sd->block_size;
		memcpy(disk->data + (start * sd->block_size), buffer, divided_length);
		return 1;
	}
	return 0;
}

const char *init_ramdisk(size_t blocks, size_t blocksize) {
	char name[16];
	if (make_unique_device_name("ram", name, sizeof(name))) {
		uint8_t *data = kmalloc(blocks * blocksize);
		if (data == NULL) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}
		ramdisk_t rd;
		rd.data = data;
		rd.name = strdup(name);
		if (!rd.name) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}
		rd.blocksize = blocksize;
		rd.blocks = blocks;
		if (disks == NULL) {
			disks = hashmap_new(sizeof(ramdisk_t), 0, 86545653, 684395435983, rd_hash, rd_compare, NULL, NULL);
			if (!disks) {
				fs_set_error(FS_ERR_OUT_OF_MEMORY);
				return NULL;
			}
		}
		hashmap_set(disks, &rd);
		storage_device_t *sd = kmalloc(sizeof(storage_device_t));
		if (!sd) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}
		sd->opaque1 = 0;
		sd->opaque2 = rd.name;
		sd->cache = NULL;
		sd->blockread = rd_block_read;
		sd->blockwrite = rd_block_write;
		sd->size = rd.blocks;
		strlcpy(sd->name, rd.name, 16);
		sd->block_size = rd.blocksize;
		register_storage_device(sd);
		return rd.name;
	}
	return NULL;
}

const char *init_ramdisk_from_memory(uint8_t *memory, size_t blocks, size_t blocksize) {
	char name[16];
	if (make_unique_device_name("ram", name, sizeof(name))) {
		ramdisk_t rd;
		rd.data = memory;
		rd.name = strdup(name);
		if (!rd.name) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}
		rd.blocksize = blocksize;
		rd.blocks = blocks;
		if (disks == NULL) {
			disks = hashmap_new(sizeof(ramdisk_t), 0, 86545653, 684395435983, rd_hash, rd_compare, NULL, NULL);
			if (!disks) {
				fs_set_error(FS_ERR_OUT_OF_MEMORY);
				return NULL;
			}
		}
		hashmap_set(disks, &rd);
		storage_device_t *sd = kmalloc(sizeof(storage_device_t));
		if (!sd) {
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return NULL;
		}
		sd->opaque1 = 0;
		sd->opaque2 = rd.name;
		sd->cache = NULL;
		sd->blockread = rd_block_read;
		sd->blockwrite = rd_block_write;
		sd->size = rd.blocks;
		strlcpy(sd->name, rd.name, 16);
		sd->block_size = rd.blocksize;
		register_storage_device(sd);
		return rd.name;
	}
	return NULL;
}

const char *init_ramdisk_from_storage(const char *storage) {
	storage_device_t *sd_src = find_storage_device(storage);
	if (!sd_src) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}
	const char *rd = init_ramdisk(sd_src->size, sd_src->block_size);
	if (!rd) {
		return NULL;
	}
	storage_device_t *sd_dst = find_storage_device(rd);
	if (!sd_dst) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return NULL;
	}
	uint32_t blocks = 1024;
	uint32_t buffer_size = sd_src->block_size * blocks;
	uint8_t *buffer = kmalloc(buffer_size);
	if (!buffer) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return NULL;
	}
	uint32_t blocks_left = sd_src->size;
	uint64_t n = 0;
	while (blocks_left > 0) {
		uint32_t to_read = blocks;
		if (to_read > blocks_left) {
			to_read = blocks_left;
		}
		if (!read_storage_device(sd_src->name, n, sd_src->block_size * to_read, buffer)) {
			kfree_null(&buffer);
			return NULL;
		}
		if (!write_storage_device(sd_dst->name, n, sd_src->block_size * to_read, buffer)) {
			kfree_null(&buffer);
			return NULL;
		}
		n += to_read;
		blocks_left -= to_read;
	}
	kfree_null(&buffer);
	return sd_dst->name;
}

static voidpf zlib_alloc(voidpf opaque, uInt items, uInt size) {
	size_t n = (size_t) items * (size_t) size;
	return kmalloc(n);
}

static void zlib_free(voidpf opaque, voidpf addr) {
	if (addr != NULL) {
		kfree(addr);
	}
}

bool decompress_gzip(uint8_t *compressed_image, size_t compressed_size, uint8_t** out, uint32_t* out_size) {
	if (compressed_image == NULL) {
		return false;
	}

	if (compressed_size < 18) {
		return false;
	}

	if (!(compressed_image[0] == 0x1F && compressed_image[1] == 0x8B && compressed_image[2] == 0x08)) {
		return false;
	}

	const uint8_t *tail = compressed_image + compressed_size - 4;
	*out_size = (uint32_t) tail[0] | ((uint32_t) tail[1] << 8) | ((uint32_t) tail[2] << 16) | ((uint32_t) tail[3] << 24);

	if (*out_size == 0) {
		return false;
	}

	*out = kmalloc(*out_size);
	if (*out == NULL) {
		return false;
	}

	z_stream zs;
	memset(&zs, 0, sizeof(zs));
	zs.zalloc = zlib_alloc;
	zs.zfree = zlib_free;
	zs.opaque = Z_NULL;
	zs.next_in = (Bytef *) compressed_image;
	zs.avail_in = (uInt) compressed_size;
	zs.next_out = (Bytef *) *out;
	zs.avail_out = (uInt) *out_size;

	int rc = inflateInit2(&zs, 16 + MAX_WBITS);
	if (rc != Z_OK) {
		kfree(*out);
		return false;
	}
	rc = inflate(&zs, Z_FINISH);
	if (rc != Z_STREAM_END) {
		inflateEnd(&zs);
		kfree(*out);
		return false;
	}
	const size_t out_len = (size_t) zs.total_out;
	inflateEnd(&zs);
	if (out_len == 0) {
		kfree(*out);
		return false;
	}
	return true;
}

bool mount_initial_ramdisk(uint8_t *compressed_image, size_t compressed_size) {

	uint32_t isize;
	uint8_t* out;
	if (!decompress_gzip(compressed_image, compressed_size, &out, &isize)) {
		preboot_fail("Failed to decompress initial ramdisk");
	}

	const size_t block_size = 2048;
	if ((isize % block_size) != 0) {
		preboot_fail("Ramdisk is not an integer multiple of 2048 bytes");
	}

	const size_t blocks = isize / block_size;
	const char *rd_name = init_ramdisk_from_memory((uint8_t *) out, blocks, block_size);
	if (rd_name == NULL) {
		preboot_fail("Failed to register initial ramdisk device");
	}
	if (!filesystem_mount("/", rd_name, "iso9660")) {
		preboot_fail("Failed to register initial ramdisk device");
	}

	return true;
}