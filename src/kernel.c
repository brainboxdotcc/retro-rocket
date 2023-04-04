#include <kernel.h>
#include <limine-requests.h>

void kmain()
{
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

	/*char test[64];
	kprintf("1: 50 %s\n", float_to_string(50, test, 64, 4));
	kprintf("2: 123.456 %s\n", float_to_string(123.456, test, 64, 4));
	kprintf("3: 5050.05431556 %s\n", float_to_string(5050.05431556, test, 64, 4));
	kprintf("4: 12343254343.75 %s\n", float_to_string(12343254343.75, test, 64, 4));
	kprintf("5: 0.0043 %s\n", float_to_string(0.0043, test, 64, 4));
	kprintf("7: 0.52 %s\n", float_to_string(0.52, test, 64, 4));
	kprintf("5: 0.623 %s\n", float_to_string(0.623, test, 64, 4));*/

	proc_init();
}
