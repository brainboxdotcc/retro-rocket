#include <kernel.h>

console first_console;
console* current_console = NULL;
spinlock init_barrier = 0;
u8 kmain_entered = 0;
u8 cpunum = 1;

void kmain_ap()
{
	while (!kmain_entered);
	idt_setup();

	/* Wait on spinlock, without trying to acquire it.
	 * If all the waiting APs try to aquire it, we get deadlock once the BSP releases it!
	 */
	while (init_barrier);

	kprintf("%d ", cpu_id());
	cpunum++;

	asm volatile("sti");

	init_lapic_timer(50);

	proc_loop();
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
	printf("64-bit SMP kernel booting, %d processors detected\n", hydrogen_info->proc_count);

	idt_setup();
	init_error_handler();
	
	/* NB: Can't use kmalloc/kfree until heap_init is called.
	 * This depends upon paging.
	 */
	heap_init();

	asm volatile("sti");

	int in;
	for (in = 0; in < 16; in++)
	{
		ioapic_redir_unmask(in);
	}

	/* These install IRQ handlers and require IOAPIC to have unmasked and mapped them */
	init_timer(50);
	init_basic_keyboard();
	ide_initialise();

	init_filesystem();
	init_iso9660();
	iso9660_attach(find_first_cdrom(), "/");
	init_fat32();
	fat32_attach(find_first_harddisk(), "/harddisk");
	init_devfs();
	init_debug();

	init_pci();

	kprintf("Loading initial process...\n");


	struct process* init = proc_load("/programs/init", (struct console*)current_console);
	if (!init)
	{
		kprintf("/programs/init missing!\n");
	}

	kprintf("Processors Booting: %d ", cpu_id());

	unlock_spinlock(&init_barrier);

	init_lapic_timer(50);

	while (cpunum < hydrogen_info->proc_count) asm volatile("hlt");

	kprintf("OK\nLaunching /programs/init...\n");

	proc_show_list();

	proc_loop();
}
