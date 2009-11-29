#include "../include/kernel.h"

void kmain(MultiBoot* mb, u64 stackaddr)
{
	initconsole();
	setforeground(COLOUR_LIGHTYELLOW);
	printf("Sixty-Four ");
	setforeground(COLOUR_WHITE);
	printf("kernel booting from %s...\n", mb->bootloadername);

	asm volatile("cli; hlt");
}
