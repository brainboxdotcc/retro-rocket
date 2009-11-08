#include "../include/io.h"
#include "../include/kernel.h"
#include "../include/video.h"
#include "../include/printf.h"
#include "../include/interrupts.h"
#include "../include/errorhandler.h"
#include "../include/keyboard.h"
#include "../include/timer.h"
#include "../include/paging.h"
#include "../include/kmalloc.h"

#define MULTIBOOT_MAGIC 0x2BADB002

console* current_console = NULL;

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void kmain(void* mbd, unsigned int magic)
{
	u32int memorysize = 0;
	if (magic == MULTIBOOT_MAGIC)
	{
		init_gdt();
		init_idt();
		init_error_handler();
		init_basic_keyboard();
		memorysize = init_paging(mbd);
		init_timer(50);
		interrupts_on();
	}

	console* cons = (console*)kmalloc(sizeof(console));
	initconsole(cons);
	current_console = cons;

	if (magic != MULTIBOOT_MAGIC)
	{
		printf("Invalid magic number %x from multiboot. System halted.\n", magic);
		blitconsole(current_console);
		wait_forever();
	}
	else
	{    
		/* You could either use multiboot.h */
		/* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
		/* or do your offsets yourself. The following is merely an example. */ 
		//char * boot_loader_name =(char*) ((long*)mbd)[16];

		printf("Sixty-Four kernel booting from %s...\n%dMb usable RAM detected.\n", (const char*)((long*)mbd)[16], memorysize / 1024 / 1024);

		asm volatile("int $50");
		wait_forever();
	}
}

