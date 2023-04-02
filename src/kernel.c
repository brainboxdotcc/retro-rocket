#include <kernel.h>
#include <limine-requests.h>

void kmain()
{
	enable_fpu();
	initconsole();
	heap_init();
	detect_cores();
	idt_setup();
	init_error_handler();
	init_pci();
	clock_init();
	init_lapic_timer(50);
	init_devicenames();
	init_basic_keyboard();
	ide_initialise();
	init_ahci();
	init_filesystem();
	init_iso9660();
	init_devfs();
	init_fat32();

	if (!filesystem_mount("/", "cd0", "iso9660")) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}
	filesystem_mount("/devices", NULL, "devfs");
	filesystem_mount("/harddisk", "hd0", "fat32");

	init_debug();

	rtl8139_init();

	proc_init();
}
