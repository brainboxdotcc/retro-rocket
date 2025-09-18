#include <kernel.h>

static audio_device_t* audio_devices = NULL;

static audio_file_loader_t* audio_loaders = NULL;

bool register_audio_device(audio_device_t* newdev) {
	/* Add the new device to the start of the list */
	newdev->next = (struct audio_device_t*)audio_devices;
	audio_devices = newdev;
	return true;
}

audio_device_t* find_audio_device(const char* name) {
	audio_device_t* cur = audio_devices;
	for(; cur; cur = (audio_device_t*)cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

audio_device_t* find_first_audio_device() {
	return audio_devices;
}

bool has_suffix_icase(const char *str, const char *suffix) {
	size_t len_str = strlen(str);
	size_t len_suf = strlen(suffix);

	if (len_str < len_suf) {
		return false;
	}

	str += len_str - len_suf;
	for (size_t i = 0; i < len_suf; i++) {
		if (tolower((unsigned char)str[i]) != tolower((unsigned char)suffix[i])) {
			return false;
		}
	}
	return true;
}

bool register_audio_loader(audio_file_loader_t* loader) {
	loader->next = (struct audio_file_loader_t*)audio_loaders;
	audio_loaders = loader;
	return true;
}

bool deregister_audio_loader(audio_file_loader_t* loader) {
	if (!loader) {
		return false;
	}

	audio_file_loader_t* prev = NULL;
	audio_file_loader_t* cur = audio_loaders;

	while (cur) {
		if (cur == loader) {
			if (prev) {
				prev->next = cur->next;
			} else {
				audio_loaders = (struct audio_file_loader_t*)cur->next;
			}
			return true;
		}
		prev = cur;
		cur = (audio_file_loader_t*)cur->next;
	}

	return false;
}


bool try_load_audio(const char* filename,const void* indata, size_t insize, void** outdata, size_t* outsize) {
	audio_file_loader_t* cur = audio_loaders;
	for(; cur; cur = (audio_file_loader_t*)cur->next) {
		if (cur->try_load_audio(filename, indata, insize, outdata, outsize)) {
			return true;
		}
	}
	return false;
}

void audio_init() {
	audio_file_loader_t* wav_loader = kmalloc(sizeof(audio_file_loader_t));
	if (!wav_loader) {
		return;
	}
	wav_loader->next = NULL;
	wav_loader->try_load_audio = wav_from_memory;
	wav_loader->opaque = NULL;
	register_audio_loader(wav_loader);
}

bool audio_file_load(const char *filename, void **out_ptr, size_t *out_bytes) {
	if (!out_ptr || !out_bytes || !filename) {
		return false;
	}
	fs_directory_entry_t *entry = fs_get_file_info(filename);
	if (!entry || (entry->flags & FS_DIRECTORY) != 0) {
		return false;
	}
	uint8_t *data = kmalloc(entry->size);
	if (!data) {
		return false;
	}
	if (!fs_read_file(entry, 0, entry->size, data)) {
		kfree(data);
		return false;
	}
	bool result = try_load_audio(filename, data, entry->size, out_ptr, out_bytes);
	kfree(data);
	return result;
}