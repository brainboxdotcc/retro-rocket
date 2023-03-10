#include <kernel.h>

static symbol_t* symbol_table = NULL;

symbol_t* get_sym_table()
{
	return symbol_table;
}

void DumpHex(unsigned char* address, u64 length)
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
	u32 filesize = symfile->size;
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
		u32 counter = 0;
		u32 offset = 0;
		u32 symcount = 0;
		u32 sizebytes = 0;
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

			u32 length = strlen(symbol) + 1;

			if (*type == 'T')
			{
				thisentry->name = (char*)kmalloc(length);
				memcpy(thisentry->name, symbol, length);
				thisentry->address = hextoint(symbol_address);
				thisentry->type = *type;
				symbol_t* next = (symbol_t*)kmalloc(sizeof(symbol_t));
				next->next = NULL;
				thisentry->next = next;
				thisentry = thisentry->next;
			}
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

const char* findsymbol(u64 address, u64* offset)
{
	/* Only the bottom 32 bits of any address is respected here, because the symbol in the
	 * sym file might be relocated (higher half) from what is the real address.
	 */
	address = address & 0xffffffff;
	symbol_t* walksyms = symbol_table;
	u64 lastsymaddr = symbol_table->address;
	symbol_t* lastsym = symbol_table;
	for (; walksyms->next; walksyms = walksyms->next)
	{
		/* This check between the last address and this address works because the
		 * symbol list in /kernel.sym is sorted by address, lowest first. This makes
		 * it simpler to load and process the list at runtime by doing the donkey-
		 * work at compile-time.
		 */
		if (address >= lastsymaddr && address <= (walksyms->address & 0xffffffff))
		{
			*offset = address - lastsymaddr;
			return lastsym->name;
		}
		lastsym = walksyms;
		lastsymaddr = walksyms->address & 0xffffffff;
	}
	return NULL;
}

void backtrace()
{
	stack_frame_t *frame;
	asm volatile("movq %%rbp,%0" : "=r"(frame));
	//frame = (stack_frame_t *)regs->ebp;
	u64 page = (u64) frame & 0xFFFFFFFFFFFFF000ull;
	u64 offset = 0;
	const char* name = NULL;

	if (!symbol_table)
	{
		kprintf("No symbols available for backtrace\n");
		return;
	}

	/* Stack frame loop inspired by AlexExtreme's stack trace in Exclaim */
	setforeground(current_console, COLOUR_LIGHTGREEN);
	while(frame && ((u64)frame & 0xFFFFFFFFFFFFF000ull) == page)
	{
		name = findsymbol((u64)frame->addr, &offset);
		kprintf("\tat %s()+0%08x [0x%016x]\n",  name ? name : "[???]", offset, frame->addr);
		frame = frame->next;
	}
	setforeground(current_console, COLOUR_WHITE);
}

