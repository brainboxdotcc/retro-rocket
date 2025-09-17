#include <kernel.h>

static audio_device_t* audio_devices = NULL;

bool register_audio_device(audio_device_t* newdev) {
	/* Add the new filesystem to the start of the list */
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
