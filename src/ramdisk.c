#include <kernel.h>

typedef struct ramdisk_t {
	const char* name;
	uint8_t* data;
	size_t blocks;
	size_t blocksize;
} ramdisk_t;

static struct hashmap* disks = NULL;

int rd_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
	const ramdisk_t* rd1 = a;
	const ramdisk_t* rd2 = b;
	return strcmp(rd1->name, rd2->name);
}

uint64_t rd_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const ramdisk_t* rd = item;
	return hashmap_sip(rd->name, strlen(rd->name), seed0, seed1);
}

int rd_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}
	ramdisk_t find_disk = { .name = sd->name };
	ramdisk_t* disk = hashmap_get(disks, &find_disk);
	if (disk) {
		uint32_t divided_length = bytes / sd->block_size;
		if (divided_length == 0) {
			divided_length = 1;
		}
		divided_length *= sd->block_size;
		memcpy(buffer, disk->data + (start * sd->block_size), divided_length);
		return 1;
	}
	return 0;
}

int rd_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer)
{
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}
	ramdisk_t find_disk = { .name = sd->name };
	ramdisk_t* disk = hashmap_get(disks, &find_disk);
	if (disk) {
		uint32_t divided_length = bytes / sd->block_size;
		if (divided_length == 0) {
			divided_length = 1;
		}
		divided_length *= sd->block_size;
		memcpy(disk->data + (start * sd->block_size), buffer, divided_length);
		return 1;
	}
	return 0;
}

const char* init_ramdisk(size_t blocks, size_t blocksize)
{
	char name[16];
	if (make_unique_device_name("ram", name)) {
		uint8_t* data = kmalloc(blocks * blocksize);
		if (data == NULL) {
			dprintf("Not enough memory to allocate a ramdisk of size %d\n", blocks * blocksize);
			return NULL;
		}
		ramdisk_t rd;
		rd.data = data;
		rd.name = strdup(name);
		rd.blocksize =blocksize;
		rd.blocks = blocks;
		if (disks == NULL) {
			disks = hashmap_new(sizeof(ramdisk_t), 0, 86545653, 684395435983, rd_hash, rd_compare, NULL, NULL);
		}
		hashmap_set(disks, &rd);
		storage_device_t* sd = kmalloc(sizeof(storage_device_t));
		sd->opaque1 = 0;
		sd->opaque2 = (void*)rd.name;
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

const char* init_ramdisk_from_storage(const char* storage)
{
	storage_device_t* sd_src = find_storage_device(storage);
	const char* rd = init_ramdisk(sd_src->size, sd_src->block_size);
	if (!rd) {
		return NULL;
	}
	storage_device_t* sd_dst = find_storage_device(rd);
	if (!sd_src || !sd_dst) {
		return NULL;
	}
	uint32_t blocks = 16;
	uint32_t buffer_size = sd_src->block_size * blocks;
	uint8_t* buffer = kmalloc(buffer_size);
	uint32_t blocks_left = sd_src->size;
	uint64_t n = 0;
	while (blocks_left > 0) {
		uint32_t to_read = blocks;
		if (to_read > blocks_left) {
			to_read = blocks_left;
		}
		read_storage_device(sd_src->name, n, sd_src->block_size * to_read, buffer);
		write_storage_device(sd_dst->name, n, sd_src->block_size * to_read, buffer);
		n += to_read;
		blocks_left -= to_read;
	}
	kfree(buffer);
	return sd_dst->name;
}
