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
		init_timer(50);
		interrupts_on();
		initialise_tasking();
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
		printf("Sixty-Four kernel booting from %s...\n%dMb usable RAM detected.\n", (const char*)((long*)mbd)[16], memorysize / 1024 / 1024);

		ide_initialise();
		init_filesystem();
		init_iso9660();
		init_debug();

		/*iso9660* iso = iso_mount_volume(0);

		u32int directory_entries;
		LINKED_LIST_COUNT(FS_DirectoryEntry*, iso->root, directory_entries);

		FS_DirectoryEntry* n;
		printf("VFS dir of %d files:\n", directory_entries);
		for(n = iso->root; n->next; n = n->next)
			printf("\t%s: size=%d flags=0x%02x\n", n->filename, n->size, n->flags);

		printf("iso_change_directory() to 'boot': %s\n", iso_change_directory(iso, "boot") ? "success" : "failure");
		printf("iso_change_directory() to 'grub': %s\n", iso_change_directory(iso, "grub") ? "success" : "failure");
		char* filebuf = (char*)kmalloc(10240);

		printf("iso_read_file(): %s\n", iso_read_file(iso, "menu.lst", 0, 104, filebuf) ? "success" : "failure");

		DumpHex(filebuf, 101);

		kfree(filebuf);

		FREE_LINKED_LIST(FS_DirectoryEntry*, iso->root);
		kfree(iso);*/

		int ret = fork(1);
		printf("Fork: %d\n", ret);
		printf("Tabs\tOne\tTwo\tThree\n");
		printf("Tabst\tOne\tThree\tFour\n");

		wait_forever();
	}
}

