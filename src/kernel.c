#include <kernel.h>
#include <initialisation-functions.h>

extern volatile struct limine_kernel_file_request rr_kfile_req;

void kmain()
{
	init();

	bool live_cd = false, root_fs_mounted = false;
	if (rr_kfile_req.response) {
		struct limine_kernel_file_response* kernel_info = rr_kfile_req.response;
		live_cd = (kernel_info->kernel_file->media_type == LIMINE_MEDIA_TYPE_OPTICAL);
	}
	if (live_cd) {
		kprintf("Booting into live CD mode...\n");
		root_fs_mounted = filesystem_mount("/", "cd0", "iso9660");
	} else {
		kprintf("Booting from hard disk installation...\n");
		root_fs_mounted = filesystem_mount("/", "hd0", "rfs");
	}
	if (!root_fs_mounted) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}

	/*load_module("ac97");

	void* wav_buffer;
	size_t wav_size;
	if (audio_wav_load("/system/webserver/media/retro-revival.wav", &wav_buffer, &wav_size)) {
		mixer_stream_t *music = mixer_create_stream();
		mixer_set_gain(music, 240);
		mixer_push(music, wav_buffer, wav_size_to_samples(wav_size));
		kfree(wav_buffer);
	}*/

	init_process();
}
