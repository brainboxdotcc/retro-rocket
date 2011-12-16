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
		kprintf("Init size: %d\n\n", get_init_size());


		const char* program = strdup(
			"100 print \"subroutine\"\n");

 		ubasic_init(program);
		do
		{
			ubasic_run();
		} while (!ubasic_finished());


		init_process_manager();

		asm volatile("int $50" : : "a"(SYS_FSWITCH));
		start_initial_task();

		proc_set_semaphore();

		//load_elf("/sh");
		/*int fd = _open("/kernel.sym", _O_RDONLY);
		if (fd == -1)
		{
			kprintf("File open error\n");
		}
		else
		{
			kprintf("File opened with fd=%d\n", fd);
			while (!_eof(fd))
			{
				unsigned char z[2090];
				int nread = _read(fd, z, 2089);
				if (nread < 1)
				{
					kprintf("\nRead error\n");
					break;
				}
				z[nread] = 0;
				kprintf("%s", z);
			}
			kprintf("\nFile EOF\n");
			if (_close(fd))
			{
				kprintf("File close error\n");
			}
		}
		kprintf("Done\n");*/

		kprintf("Epic done!\n");

		wait_forever();
	}
}
