#include "../include/debugger.h"
#include "../include/printf.h"
#include "../include/video.h"
#include "../include/iso9660.h"
#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"

typedef struct symbol
{
	char* name;
	u32int address;
	u8int type;
	struct symbol* next;
} symbol_t;

void DumpHex(unsigned char* address, u32int length)
{
	int index = 0;
	for(; index < length; index += 16)
	{
		printf("%04x: ", index);
		int hex = 0;
		for (; hex < 16; ++hex)
			printf("%02x ", address[index + hex]);
		putstring(current_console, " | ");
		for (hex = 0; hex < 16; ++hex)
			put(current_console, (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);

		put(current_console, '\n');
	}
}

void symbol_fail()
{
	setforeground(current_console, COLOUR_DARKRED);
	printf("Warning: Could not load /kernel.sym from boot device.\n");
	printf("Debug symbols will be unavailable if there is a kernel panic.\n");
	setforeground(current_console, COLOUR_WHITE);
}

void init_debug()
{
	iso9660* iso = iso_mount_volume(0);
	if (!iso)
	{
		symbol_fail();
		return;
	}
	FS_DirectoryEntry* n;
	u32int filesize = 0;
	for(n = iso->root; n->next; n = n->next)
	{
		if (n->flags == 0 && !strcmp(n->filename, "kernel.sym"))
		{
			filesize = n->size;
			break;
		}
	}
	if (filesize == 0)
	{
		symbol_fail();
		kfree(iso->root);
		kfree(iso);
		return;
	}
	else
	{
		unsigned char* filecontent = (unsigned char*)kmalloc(filesize);
		if (!iso_read_file(iso, "kernel.sym", 0, filesize, filecontent))
		{
			symbol_fail();
			kfree(iso->root);
			kfree(iso);
			return;
		}
		/* We now have the symbols in the filecontent buffer. Parse them to linked list,
		 * and free the buffer.
		 */
		unsigned char* ptr = filecontent;
		char symbol_address[32];
		char type[2];
		char symbol[1024];
		u32int counter = 0;
		u32int offset = 0;
		u32int symcount = 0;
		u32int sizebytes = 0;
		symbol_t* symbol_table = (symbol_t*)kmalloc(sizeof(symbol_t));
		symbol_t* thisentry = symbol_table;

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

		kfree(iso->root);
		kfree(iso);
		kfree(filecontent);
		printf("Read %d symbols from /kernel.sym (%d bytes)\n", symcount, sizebytes);
	}
}

