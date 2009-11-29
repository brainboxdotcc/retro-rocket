#include "../include/kernel.h"

void kmain(MultiBoot* mb, u64 stackaddr)
{
	initconsole();
	put('E');
	putstring("This is a test of a longer string");
	asm volatile("cli; hlt");
}
