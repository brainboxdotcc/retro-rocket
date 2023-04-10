#include <kernel.h>
#include <limine-requests.h>

init_func_t init[] = {
	init_console, init_heap, init_cores, init_idt, init_error_handler, init_pci,
	init_realtime_clock, init_lapic_timer, init_devicenames, init_keyboard, init_ide,
	init_ahci, init_filesystem, init_iso9660, init_devfs, init_fat32,
	NULL,
};

void kmain()
{
	for (init_func_t* func = init; *func; ++func) {
		(*func)();
	}

	if (!filesystem_mount("/", "cd0", "iso9660")) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}
	filesystem_mount("/devices", NULL, "devfs");
	const char* rd = init_ramdisk_from_storage("hd0");
	if (rd) {
		filesystem_mount("/harddisk", rd, "fat32");
	}

	init_debug();
	init_rtl8139();

	if (fs_delete_file("/harddisk/test2.txt")) {
		dprintf("It worked!\n");
	}

	init_process();
}
