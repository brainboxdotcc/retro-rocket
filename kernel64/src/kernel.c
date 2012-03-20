#include "../include/kernel.h"

void kmain_ap()
{
	asm volatile("cli; hlt");
}

void kmain()
{
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Sixty-Four ");
	setforeground(COLOUR_WHITE);
	printf("kernel booting from %s...\n", "Hydrogen");
	printf("%d MB RAM detected\n", 64);
	
	asm volatile("int $49");

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	wait_forever();
}
