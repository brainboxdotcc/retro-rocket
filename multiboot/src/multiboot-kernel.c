#include "../include/io.h"
#include "../include/kernel.h"
#include "../include/video.h"
#include "../include/printf.h"
#include "../include/interrupts.h"
#include "../include/errorhandler.h"
#include "../include/keyboard.h"
#include "../include/timer.h"

#define MULTIBOOT_MAGIC 0x2BADB002

console* current_console;

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void kmain(void* mbd, unsigned int magic)
{
	console cons;
	current_console = &cons;
	initconsole(current_console);
	clearscreen(current_console);

	init_gdt();
	init_idt();
	init_timer(50);
	interrupts_on();

	if (magic != MULTIBOOT_MAGIC)
	{
		putstring(current_console, "Invalid magic number from multiboot. System halted.\n");
		for(;;);
	}
	    
	/* You could either use multiboot.h */
	/* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
	/* or do your offsets yourself. The following is merely an example. */ 
	//char * boot_loader_name =(char*) ((long*)mbd)[16];

	init_error_handler();
	init_basic_keyboard();

	printf("Sixty-Four kernel booting from %s...\n", (const char*)((long*)mbd)[16]);

	//blitconsole(current_console);
	//
	asm volatile("int $50");

	for(;;)
	{
		blitconsole(current_console);
	}
}

