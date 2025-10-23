#include <kernel.h>

void kmain() {
	init();

	bool live_cd = booted_from_cd(), root_fs_mounted = false;
	uint8_t* gzipped_iso;
	size_t iso_size;
	if (find_limine_module("/root.iso.gz", &gzipped_iso, &iso_size)) {
		kprintf("Booting into live USB mode...\n");
		mount_initial_ramdisk(gzipped_iso, iso_size);
	} else {
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
	}

	init_process();
}
