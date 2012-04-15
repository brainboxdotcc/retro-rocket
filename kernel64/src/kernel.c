#include <kernel.h>

console first_console;
console* current_console = NULL;
spinlock init_barrier;

void kmain_ap()
{
	//idt_setup();
	while (1) { asm volatile("hlt"); }
}

void kmain()
{
	current_console = &first_console;
	init_spinlock(&init_barrier);
	lock_spinlock(&init_barrier);
	initconsole(current_console);
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

	int in = 16;
	for (in = 0; in < 16; in++)
	{
		ioapic_redir_unmask(in);
	}

	/* These install IRQ handlers and require IOAPIC to have unmasked and mapped them */
	init_timer(250);
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

	unlock_spinlock(&init_barrier);

	sleep(3);

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	//wait_forever();
	while (1) { continue; }
}
