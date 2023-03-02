#include "../include/kernel.h"

void kmain(MultiBoot* mb, u64 stackaddr, u64 memorymb)
{
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Sixty-Four ");
	setforeground(COLOUR_WHITE);
	printf("kernel booting from %s...\n", mb->bootloadername);
	printf("%d MB RAM detected\n", memorymb);
	initialise_paging();

	if (!detect_apic())
	{
		printf("Could not detect local APIC. System initialisation halted.\n");
		wait_forever();
	}
	
	asm volatile("int $49");

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	wait_forever();
}
