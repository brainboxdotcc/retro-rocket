#include <debugger.h>
#include <kprintf.h>
#include <video.h>
#include <iso9660.h>
#include <filesystem.h>
#include <string.h>
#include <kmalloc.h>
#include <memcpy.h>
#include <interrupts.h>

static symbol_t* symbol_table = NULL;

symbol_t* get_sym_table()
{
	return symbol_table;
}

void DumpHex(unsigned char* address, u32int length)
{
	int index = 0;
	for(; index < length; index += 16)
	{
		kprintf("%04x: ", index);
		int hex = 0;
		for (; hex < 16; ++hex)
			kprintf("%02x ", address[index + hex]);
		putstring(current_console, " | ");
		for (hex = 0; hex < 16; ++hex)
			put(current_console, (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);

		put(current_console, '\n');
	}
}

void symbol_fail()
{
	setforeground(current_console, COLOUR_DARKRED);
	kprintf("Warning: Could not load /kernel.sym from boot device.\n");
	kprintf("Debug symbols will be unavailable if there is a kernel panic.\n");
	setforeground(current_console, COLOUR_WHITE);
}

void init_debug()
{
	//kprintf("init_debug\n");
	FS_DirectoryEntry* symfile = fs_get_file_info("/kernel.sym");
	if (!symfile)
	{
		symbol_fail();
		return;
	}
	//kprintf("init_debug 2\n");
	u32int filesize = symfile->size;
	if (filesize == 0)
	{
		symbol_fail();
		return;
	}
	else
	{
		//kprintf("init_debug 3\n");
		unsigned char* filecontent = (unsigned char*)kmalloc(filesize);
		if (!fs_read_file(symfile, 0, filesize, filecontent))
		{
			//kprintf("init_debug 4\n");
			symbol_fail();
			kfree(filecontent);
			return;
		}
		/* We now have the symbols in the filecontent buffer. Parse them to linked list,
		 * and free the buffer.
		 */
		//kprintf("init_debug 5\n");
		unsigned char* ptr = filecontent;
		char symbol_address[32];
		char type[2];
		char symbol[1024];
		u32int counter = 0;
		u32int offset = 0;
		u32int symcount = 0;
		u32int sizebytes = 0;
		symbol_table = (symbol_t*)kmalloc(sizeof(symbol_t));
		symbol_t* thisentry = symbol_table;

		//kprintf("init_debug 6\n");

		while (offset < filesize)
		{
			counter = 0;
			while (offset < filesize && *ptr != ' ' && counter < sizeof(symbol_address) - 1)
			{
				symbol_address[counter++] = *ptr++;
				offset++;
			}
			symbol_address[counter] = 0;
			ptr++;
			offset++;

			counter = 0;
			while (offset < filesize && *ptr != ' ' && counter < 2)
			{
				type[counter++] = *ptr++;
				offset++;
			}
			type[counter] = 0;
			ptr++;
			offset++;

			counter = 0;
			while (offset < filesize && *ptr != 0x0A && counter < sizeof(symbol) - 1)
			{
				symbol[counter++] = *ptr++;
				offset++;
			}
			symbol[counter] = 0;
			ptr++;
			offset++;

			u32int length = strlen(symbol) + 1;
			thisentry->name = (char*)kmalloc(length);
			memcpy(thisentry->name, symbol, length);
			thisentry->address = hextoint(symbol_address);
			thisentry->type = *type;
			symbol_t* next = (symbol_t*)kmalloc(sizeof(symbol_t));
			next->next = NULL;
			thisentry->next = next;
			thisentry = thisentry->next;
			symcount++;
			sizebytes += sizeof(symbol_t) + length;
		}

		kfree(filecontent);
		kprintf("Read ");
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("%d ", symcount);
		setforeground(current_console, COLOUR_WHITE);
		kprintf("symbols from ");
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("/kernel.sym ");
		setforeground(current_console, COLOUR_WHITE);
		kprintf("(%d bytes)\n", sizebytes);
	}
}

const char* findsymbol(u32int address, u32int* offset)
{
	symbol_t* walksyms = symbol_table;
	u32int lastsymaddr = symbol_table->address;
	symbol_t* lastsym = symbol_table;
	for (; walksyms->next; walksyms = walksyms->next)
	{
		/* This check between the last address and this address works because the
		 * symbol list in /kernel.sym is sorted by address, lowest first. This makes
		 * it simpler to load and process the list at runtime by doing the donkey-
		 * work at compile-time.
		 */
		if (address >= lastsymaddr && address <= walksyms->address)
		{
			*offset = address - lastsymaddr;
			return lastsym->name;
		}
		lastsym = walksyms;
		lastsymaddr = walksyms->address;
	}
	return NULL;
}

void backtrace(registers_t* regs)
{
	stack_frame_t *frame;
	frame = (stack_frame_t *)regs->ebp;
	u32int page = (u32int) frame & 0xFFFFF000;
	u32int offset = 0;
	const char* name = NULL;

	if (!symbol_table)
	{
		kprintf("No symbols available for backtrace\n");
		return;
	}

	/* Stack frame loop inspired by AlexExtreme's stack trace in Exclaim */
	setforeground(current_console, COLOUR_LIGHTGREEN);
	name = findsymbol((u32int)regs->eip, &offset);
	kprintf("\tat %s+0%04x [0x%08x]\n",  name ? name : "[???]", offset, regs->eip);
	while(frame && ((u32int) frame & 0xFFFFF000) == page)
	{
		name = findsymbol((u32int)frame->addr, &offset);
		kprintf("\tat %s+0%04x [0x%08x]\n",  name ? name : "[???]", offset, frame->addr);
		frame = frame->next;
	}
	setforeground(current_console, COLOUR_WHITE);
}

