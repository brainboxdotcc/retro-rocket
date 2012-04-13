#include <kernel.h>
#include <hydrogen.h>
#include <debugger.h>
#include <spinlock.h>
#include <kmalloc.h>
#include <timer.h>
#include <ioapic.h>

/* 64-bit IDT is at 0x0, same position as realmode IDT */
u16 idt64[5] = {0xffff, 0x0000, 0x0000, 0x0000, 0x0000 };

extern void idt_init();

void kmain_ap()
{
	//idt_init();
	//asm volatile("lidtq (%0)\n"::"r"(idt64));
	//asm volatile("sti");
	while (1) { asm volatile("hlt"); }
}

void bt3()
{
	int a = 1 - 1;
	int b = 1;
	int c = b / a;
}

void bt2()
{
	bt3();
}

void backtracetest()
{
	bt2();
}

spinlock init_barrier;

void kmain()
{
	init_spinlock(&init_barrier);
	lock_spinlock(&init_barrier);
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Retro-Rocket ");
	setforeground(COLOUR_WHITE);
	printf("64-bit SMP kernel booting from %s...\n", "Hydrogen");
	printf("%d processors detected, %d IOAPICs\n", hydrogen_info->proc_count, hydrogen_info->ioapic_count);

	init_interrupts();
	idt_init();
	asm volatile("lidtq (%0)\n"::"r"(idt64));

	init_error_handler();
	heap_init();

	asm volatile("sti");

	int in = 16;
	for (in = 0; in < 16; in++)
	{
		ioapic_redir_unmask(in);
	}
	init_timer(100);
	ide_initialise();
	init_filesystem();
	init_iso9660();
	iso9660_attach(find_first_cdrom(), "/");
	init_fat32();
	fat32_attach(find_first_harddisk(), "/harddisk");
	init_devfs();
	init_debug();

	unlock_spinlock(&init_barrier);

	backtracetest();

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	//wait_forever();
	while (1) { continue; }
}
