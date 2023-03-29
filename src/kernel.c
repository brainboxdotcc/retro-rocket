#include <kernel.h>

console first_console;
console* current_console = NULL;
spinlock init_barrier = 0;
uint8_t kmain_entered = 0;
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
	init_spinlock(&init_barrier);
	lock_spinlock(&init_barrier);

	current_console = &first_console;
	initconsole(current_console);

	kmain_entered = 1;

	setforeground(current_console, COLOUR_LIGHTYELLOW);
	printf("Retro-Rocket ");
	setforeground(current_console, COLOUR_WHITE);
	printf("64-bit SMP kernel booting\n");

	/* NB: Can't use kmalloc/kfree until heap_init is called.
	 * This depends upon paging.
	 */
	heap_init();
	
	detect_cores();
	idt_setup();

	for (int in = 0; in < 16; in++)
	{
		ioapic_redir_unmask(in);
		ioapic_redir_set_precalculated(in, 0, 0x2020 + in);
	}

	asm volatile("sti");

	init_error_handler();
	init_pci();
	init_basic_keyboard();
	ide_initialise(0x1F0, 0x3F4, 0x170, 0x374, 0x000);
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

	clock_init();
	init_lapic_timer(50);

	rtl8139_init();

	kprintf("System boot time: %s\n", get_datetime_str());
	kprintf("Loading initial process...\n");

	/* Drawing test cases */
	draw_line(0, 0, screen_get_width() - 1, screen_get_height() / 2, 0xFF0000);
	draw_line(screen_get_width() - 1, screen_get_height() / 2, 0, screen_get_height() - 1, 0x0000FF);
	draw_line(0, screen_get_height() / 2, screen_get_width() - 1, screen_get_height() / 2, 0x00FF00);

	struct process* init = proc_load("/programs/init", (struct console*)current_console);
	if (!init)
	{
		kprintf("/programs/init missing!\n");
	}

	unlock_spinlock(&init_barrier);

	kprintf("Launching /programs/init...\n");

	proc_loop();
}
