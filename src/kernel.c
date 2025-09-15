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

	redefine_character(0xAA, (uint8_t[8]){0x3C, 0x7E, 0xFF, 0xE7, 0xFF, 0x66, 0x66, 0x00});
	graphics_putstring("\xAA\xAA\xAA TEST \xAA\xAA\xAA", 640, 768 / 2, 0xff8080);

	init_process();
}
