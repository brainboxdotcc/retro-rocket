#include <kernel.h>
#include <initialisation-functions.h>

extern volatile struct limine_kernel_file_request rr_kfile_req;

void kmain()
{
	init();

	if (get_active_network_device()) {
		kprintf("Bringing up network...");
		network_up();
	}

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

	/*module test;
	fs_directory_entry_t * f = fs_get_file_info("/system/modules/test.ko");
	if (f) {
		dprintf("*** MOD TEST: Got info on test.ko size %lu\n", f->size);
		void* module_content = kmalloc(f->size);
		if (fs_read_file(f, 0, f->size, module_content)) {
			dprintf("*** LOADING TEST MODULE test.ko ***\n");
			if (module_load_from_memory(module_content, f->size, &test)) {
				module_unload(&test);
			}
			dprintf("*** DONE WITH TEST MODULE test.ko ***\n");
		} else {
			dprintf("Couldnt read test.ko\n");
		}
	} else {
		dprintf("*** MOD TEST: Cant get info on test.ko\n");
	}*/

	init_process();
}
