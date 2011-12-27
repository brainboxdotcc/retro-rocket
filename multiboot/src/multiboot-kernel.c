#include "../include/io.h"
#include "../include/string.h"
#include "../include/kernel.h"
#include "../include/video.h"
#include "../include/kprintf.h"
#include "../include/interrupts.h"
#include "../include/errorhandler.h"
#include "../include/keyboard.h"
#include "../include/timer.h"
#include "../include/paging.h"
#include "../include/kmalloc.h"
#include "../include/ata.h"
#include "../include/iso9660.h"
#include "../include/filesystem.h"
#include "../include/debugger.h"
#include "../include/taskswitch.h"
#include "../include/devfs.h"
#include "../include/elf.h"
#include "../include/syscall.h"
#include "../include/ubasic.h"
#include "../include/input.h"

#define MULTIBOOT_MAGIC 0x2BADB002

console* current_console = NULL;
u32int initial_esp = NULL;
int usermode_init = 0;

static unsigned char* vid = (unsigned char*) VIDEO_MEMORY;

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void kmain(void* mbd, unsigned int magic, u32int sp)
{
	*vid = '1';
	u32int memorysize = 0;
	initial_esp = sp;
	if (magic == MULTIBOOT_MAGIC)
	{
		*vid = '2';
		init_gdt();
		*vid = '3';
		init_idt();
		*vid = '4';
		memorysize = init_paging(mbd);
		*vid = '5';
		init_error_handler();
		*vid = '6';
		init_basic_keyboard();
		*vid = '7';
		init_syscall();
		*vid = '9';
		init_timer(10);
		*vid = 'A';
	}

	console* cons = (console*)kmalloc(sizeof(console));
	initconsole(cons);
	current_console = cons;
	blitconsole(cons);

	*vid = 'B';

	interrupts_on();

	*vid = 'C';

	if (magic != MULTIBOOT_MAGIC)
	{
		kprintf("Invalid magic number %x from multiboot. System halted.\n", magic);
		wait_forever();
	}
	else
	{
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("Sixty-Four");
		setforeground(current_console, COLOUR_WHITE);
		kprintf(" kernel booting from %s...\n%dMb usable RAM detected.\n", (const char*)((long*)mbd)[16], memorysize / 1024 / 1024);

		ide_initialise();
		init_filesystem();
		init_iso9660();
		iso9660_attach(0, "/");
		init_devfs();
		init_debug();

		interrupts_on();

		const char* program = strdup(
"10 a = 3200\n\
20 foobar = 50\n\
25 bazqux = 40\n\
26 mystring$ = \"A String\"\n\
30 print \"Variables\"\n\
40 print foobar + bazqux - 1, 89, mystring$ + \" extra\"\n\
45 input inp$\n\
50 print \"You entered: \" + inp$ + \" ...What Craq\"");

 		struct ubasic_ctx* ctx = ubasic_init(program, current_console);
		do
		{
			ubasic_run(ctx);
		}
		while (!ubasic_finished(ctx));
		ubasic_destroy(ctx);

		kprintf("Epic done!\n");

		wait_forever();
	}
}
