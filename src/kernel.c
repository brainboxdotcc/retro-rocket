#include <kernel.h>

console first_console;
console* current_console = NULL;
uint8_t cpunum = 1;

volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .stack_size = (1024 * 1024 * 32),
};

volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

void kmain_ap()
{
	while (1) asm volatile("hlt");
}

void network_up()
{
	arp_init();
	ip_init();
	tcp_init();
	dhcp_discover();
	init_dns();
}

void network_down()
{
}

void kmain()
{
	current_console = &first_console;
	initconsole(current_console);

	/* NB: Can't use kmalloc/kfree until heap_init is called.
	 * This depends upon paging.
	 */
	heap_init();
	detect_cores();
	idt_setup();
	init_error_handler();
	fninit();
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

	//////

	rtl8139_init();

	kprintf("System boot time: %s\n", get_datetime_str());
	kprintf("Loading initial process...\n");

	struct process* init = proc_load("/programs/init", (struct console*)current_console);
	if (!init) {
		kprintf("/programs/init missing!\n");
	}

	kprintf("Launching /programs/init...\n");

	proc_loop();
}
