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

void _memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}


void kmain(void* mbd, unsigned int magic, u32int sp)
{
	u32int memorysize = 0;
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

		print_heapinfo();
		ide_initialise();
		init_filesystem();
		init_iso9660();
		iso9660_attach(find_first_cdrom(), "/");
		init_fat32();
		fat32_attach(find_first_harddisk(), "/harddisk");
		init_devfs();
		init_debug();

		/*MultiBoot* mb = (MultiBoot*)mbd;

		kprintf("flags=%08x mem_lower=%d mem_upper=%d boot_device=%08x mmap_addr=%08x\n", mb->flags, mb->mem_lower, mb->mem_upper, mb->boot_device, mb->mmap_addr);

		kprintf("Boot device avail: %s\n", (mb->flags & MB_BOOTDEV) ? "Yes" : "No");
		kprintf("Loader name avail: %s\n", (mb->flags & MB_BOOTLOADERNAME) ? "Yes" : "No");
		kprintf("Memory Map avail: %s\n", (mb->flags & MB_MEMMAP) ? "Yes" : "No");
		
		MB_MemMap* mm = mb->mmap_addr;

		kprintf("Memory map len: %08x\n", mb->mmap_len);

		int c = 0;
		while ((u32int)mm < mb->mmap_len + mb->mmap_addr)
		{
			kprintf("MMAP[%d] size=%08x addrlow=%08x lenlow=%08x type=%d\n",
				c++, mm->size, mm->addrlow, mm->lenlow, mm->type);

			if (mm->lenlow + mm->addrlow == 0x0)
				break;
			else
				mm = (MB_MemMap*)((u32int)mm + mm->size + sizeof(mm->size));
		}*/

		struct process* init = proc_load("/programs/init", (struct console*)current_console);
		if (!init)
		{
			kprintf("/programs/init missing!\n");
		}
		else
		{
			kprintf("Launching /programs/init...\n");
			proc_loop();
		}

		kprintf("System Halted.\n");
	}

	wait_forever();
}

