#include "../include/kernel.h"

void kmain(MultiBoot* mb, u64 stackaddr)
{
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Sixty-Four ");
	setforeground(COLOUR_WHITE);
	printf("kernel booting from %s...\n", mb->bootloadername);

	if (!detect_apic())
	{
		printf("Could not detect local APIC. System initialisation halted.\n");
		wait_forever();
	}

	asm volatile("int $49");

	printf("Would continue boot sequence, but brain hasnt got any further!\n");
	wait_forever();
}
