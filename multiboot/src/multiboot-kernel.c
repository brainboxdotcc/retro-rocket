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
#include "../include/ata.h"
#include "../include/iso9660.h"
#include "../include/filesystem.h"
#include "../include/debugger.h"
#include "../include/taskswitch.h"
#include "../include/devfs.h"

#define MULTIBOOT_MAGIC 0x2BADB002

console* current_console = NULL;
u32int initial_esp = NULL;

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void kmain(void* mbd, unsigned int magic, u32int sp)
{
	u32int memorysize = 0;
	initial_esp = sp;
	if (magic == MULTIBOOT_MAGIC)
	{
		init_gdt();
		init_idt();
		init_error_handler();
		init_basic_keyboard();
		memorysize = init_paging(mbd);
		initialise_tasking();
		init_timer(50);
		interrupts_on();
	}

	console* cons = (console*)kmalloc(sizeof(console));
	initconsole(cons);
	current_console = cons;

	if (magic != MULTIBOOT_MAGIC)
	{
		printf("Invalid magic number %x from multiboot. System halted.\n", magic);
		wait_forever();
	}
	else
	{
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		printf("Sixty-Four");
		setforeground(current_console, COLOUR_WHITE);
		printf(" kernel booting from %s...\n%dMb usable RAM detected.\n", (const char*)((long*)mbd)[16], memorysize / 1024 / 1024);

		ide_initialise();
		init_filesystem();
		init_iso9660();
		iso9660_attach(0, "/");
		init_devfs();

		init_debug();
		printf("\n");

		printf("Kernel vfs tests\n");

		u32int itemsc;
		FS_DirectoryEntry* n;
		FS_DirectoryEntry* items = fs_get_items("/boot/grub");
		printf("Result from fs_get_items(\"/boot/grub\"): 0x%08x\n", items);

		LINKED_LIST_COUNT(FS_DirectoryEntry*, items, itemsc);
		printf("VFS dir of %d files.\n", itemsc);

		items = fs_get_items("/devices");
		printf("Result from fs_get_items(\"/devices\"): 0x%08x\n", items);
		LINKED_LIST_COUNT(FS_DirectoryEntry*, items, itemsc);
		printf("VFS dir of %d files.\n", itemsc);

		items = fs_get_file_info("/boot/grub/stage2");
		printf("Result from fs_get_file_info(\"/boot/grub/stage2\"): 0x%08x\n", items);
		if (items)
			printf("name: %s size: %d lbapos: %d\n", items->filename, items->size, items->lbapos);

		wait_forever();
	}
}
