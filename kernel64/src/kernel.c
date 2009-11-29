#include "../include/kernel.h"

void kmain(MultiBoot* mb, u64 stackaddr)
{
	initconsole();
	printf("The future is %s not %s", "now", "then");
	asm volatile("cli; hlt");
}
