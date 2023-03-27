#include <kernel.h>

static symbol_t* symbol_table = NULL;

symbol_t* get_sym_table()
{
	return symbol_table;
}

void dump_hex(unsigned char* address, uint64_t length)
{
	uint64_t index = 0;
	for(; index < length; index += 16)
	{
		kprintf("%04x: ", index);
		size_t hex = 0;
		for (; hex < 16; ++hex) {
			if (index + hex < length) {
				kprintf("%02X ", address[index + hex]);
			} else {
				putstring(current_console, "   ");
			}
		}
		putstring(current_console, " | ");
		for (hex = 0; hex < 16; ++hex) {
			if (index + hex < length) {
				put(current_console, (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);
			} else {
				put(current_console, ' ');
			}
		}

		put(current_console, '\n');
	}
}

void symbol_fail()
{
	setforeground(current_console, COLOUR_DARKRED);
	putstring(current_console, "Warning: Could not load /kernel.sym from boot device.\n");
	putstring(current_console, "Debug symbols will be unavailable if there is a kernel panic.\n");
	setforeground(current_console, COLOUR_WHITE);
}

void init_debug()
{
	FS_DirectoryEntry* symfile = fs_get_file_info("/kernel.sym");
	if (!symfile) {
		symbol_fail();
		return;
	}
	uint32_t filesize = symfile->size;
	if (filesize == 0) {
		symbol_fail();
		return;
	} else {
		unsigned char* filecontent = (unsigned char*)kmalloc(filesize);
		if (!fs_read_file(symfile, 0, filesize, filecontent)) {
			symbol_fail();
			kfree(filecontent);
			return;
		}
		/* We now have the symbols in the filecontent buffer. Parse them to linked list,
		 * and free the buffer.
		 */
		unsigned char* ptr = filecontent;
		char symbol_address[32];
		char type[2];
		char symbol[1024];
		uint32_t counter = 0;
		uint32_t offset = 0;
		uint32_t symcount = 0;
		uint32_t sizebytes = 0;
		symbol_table = (symbol_t*)kmalloc(sizeof(symbol_t));
		symbol_t* thisentry = symbol_table;

		while (offset < filesize) {
			counter = 0;
			while (offset < filesize && *ptr != ' ' && counter < sizeof(symbol_address) - 1) {
				symbol_address[counter++] = *ptr++;
				offset++;
			}
			symbol_address[counter] = 0;
			ptr++;
			offset++;

			counter = 0;
			while (offset < filesize && *ptr != ' ' && counter < 2) {
				type[counter++] = *ptr++;
				offset++;
			}
			type[1] = 0;
			ptr++;
			offset++;

			counter = 0;
			while (offset < filesize && *ptr != 0x0A && counter < sizeof(symbol) - 1) {
				symbol[counter++] = *ptr++;
				offset++;
			}
			symbol[counter] = 0;
			ptr++;
			offset++;

			uint32_t length = strlen(symbol) + 1;

			if (*type == 'T') {
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
		kprintf("(%d bytes)\n", filesize);
	}
}

const char* findsymbol(uint64_t address, uint64_t* offset)
{
	/* Only the bottom 32 bits of any address is respected here, because the symbol in the
	 * sym file might be relocated (higher half) from what is the real address.
	 */
	address = address & 0xffffffff;
	symbol_t* walksyms = symbol_table;
	uint64_t lastsymaddr = symbol_table->address;
	symbol_t* lastsym = symbol_table;
	for (; walksyms->next; walksyms = walksyms->next) {
		/* This check between the last address and this address works because the
		 * symbol list in /kernel.sym is sorted by address, lowest first. This makes
		 * it simpler to load and process the list at runtime by doing the donkey-
		 * work at compile-time.
		 */
		if (address >= lastsymaddr && address <= (walksyms->address & 0xffffffff)) {
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
	uint64_t page = (uint64_t) frame & 0xFFFFFFFFFFFFF000ull;
	uint64_t offset = 0;
	const char* name = NULL;

	if (!symbol_table) {
		kprintf("No symbols available for backtrace\n");
		return;
	}

	/* Stack frame loop inspired by AlexExtreme's stack trace in Exclaim */
	setforeground(current_console, COLOUR_LIGHTGREEN);
	while (frame && ((uint64_t)frame & 0xFFFFFFFFFFFFF000ull) == page) {
		name = findsymbol((uint64_t)frame->addr, &offset);
		kprintf("\tat %s()+0%08x [0x%016x]\n",  name ? name : "[???]", offset, frame->addr);
		frame = frame->next;
	}
	setforeground(current_console, COLOUR_WHITE);
}

