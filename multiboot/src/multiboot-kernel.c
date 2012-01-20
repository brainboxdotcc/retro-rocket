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
#include "../include/fat32.h"
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

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void kmain(void* mbd, unsigned int magic, u32int sp)
{
	u32int memorysize = 0;
	initial_esp = sp;
	console* cons = NULL;
	
	if (magic == MULTIBOOT_MAGIC)
	{
		init_gdt();
		init_idt();
		memorysize = init_paging(mbd);
		init_error_handler();
		init_basic_keyboard();
		init_syscall();
		init_timer(50);
		cons = (console*)kmalloc(sizeof(console));
		initconsole(cons);
		current_console = cons;
		blitconsole(cons);
		interrupts_on();

		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("Retro Rocket");
		setforeground(current_console, COLOUR_WHITE);
		kprintf(" kernel booting from %s...\n%dMb usable RAM detected.\n", (const char*)((long*)mbd)[16], memorysize / 1024 / 1024);

		ide_initialise();
		init_filesystem();
		init_iso9660();
		iso9660_attach(find_first_cdrom(), "/");
		init_fat32();
		fat32_attach(find_first_harddisk(), "/harddisk");
		init_devfs();
		init_debug();

		struct process* proc = proc_load("/programs/init", (struct console*)current_console);
		kprintf("Launching /programs/init...\n");
		proc_loop();

		kprintf("System Halted.\n");
	}

	wait_forever();
}

