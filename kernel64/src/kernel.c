#include "../include/kernel.h"

console current_console;

void kmain(MultiBoot* mb, u64 stackaddr)
{
	initconsole(&current_console);
	printf("Epic win.\n");
	asm volatile("cli; hlt");
}
