#include <io.h>
#include <string.h>
#include <kernel.h>
#include <video.h>
#include <kprintf.h>
#include <interrupts.h>
#include <errorhandler.h>
#include <keyboard.h>
#include <timer.h>
#include <paging.h>
#include <kmalloc.h>
#include <ata.h>
#include <iso9660.h>
#include <fat32.h>
#include <filesystem.h>
#include <debugger.h>
#include <taskswitch.h>
#include <devfs.h>
#include <elf.h>
#include <syscall.h>
#include <ubasic.h>
#include <input.h>
#include <multiboot.h>

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

