#include <kernel.h>

static void debug_update_cb(fs_directory_entry_t *ent) {
	ent->size = dprintf_size();
}

static bool debug_read_cb(uint64_t start, uint32_t length, unsigned char *buffer) {
	char *log = dprintf_buffer_snapshot();
	uint64_t log_length = strlen(log);

	if (start + length > log_length) {
		kfree_null(&log);
		fs_set_error(FS_ERR_SEEK_PAST_END);
		return false;
	}
	memcpy(buffer, log + start, length);
	kfree_null(&log);
	return true;
}

void init_debuglog(void) {
	devfs_register_text("debug", debug_update_cb, debug_read_cb);
}