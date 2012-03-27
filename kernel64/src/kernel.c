#include <kernel.h>
#include <hydrogen.h>
#include <debugger.h>
#include <spinlock.h>

u16 idt64[5] = {0xffff, 0x0000, 0x0000, 0x0000, 0x0000 };

extern void idt_init();

void kmain_ap()
{
	asm volatile("cli; hlt");
}

void kmain()
{
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Retro-Rocket ");
	setforeground(COLOUR_WHITE);
	printf("kernel booting from %s...\n", "Hydrogen");
	printf("%d MB RAM detected\n", hydrogen_info->mmap_count);

	idt_init();

	asm volatile("lidtq (%0)\n"::"r"(idt64));
	
	asm volatile("int $49");

	DumpHex(hydrogen_mmap, 256);

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	wait_forever();
}
