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
		//initialise_tasking();
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

		int fd = _open("/kernel.sym", _O_RDONLY);
		if (fd == -1)
		{
			printf("File open error\n");
		}
		else
		{
			printf("File opened with fd=%d\n", fd);
			while (!_eof(fd))
			{
				unsigned char z[2090];
				int nread = _read(fd, z, 2089);
				if (nread < 1)
				{
					printf("\nRead error\n");
					break;
				}
				z[nread] = 0;
				printf("%s", z);
			}
			printf("\nFile EOF\n");
			if (_close(fd))
			{
				printf("File close error\n");
			}
		}
		printf("Done\n");

		wait_forever();
	}
}
