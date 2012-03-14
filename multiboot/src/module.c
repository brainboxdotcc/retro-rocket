#include <module.h>
#include <kernel.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <paging.h>
#include <filesystem.h>
#include <debugger.h>
#include <memcpy.h>

Module* modlist = NULL;
Module* paged = NULL;
KernelInfo* ki = NULL;

int page_module(Module* mod)
{
	if (paged == mod)
		return 0;
	else
	{
		memcpy((void*)MODSTART, mod->progbits, mod->size);
		paged = mod;
	}
	return mod->size;
}

u32int find_symbol(const char* symbol)
{
	kprintf("Module looking up symbol '%s'\n", symbol);
	return NULL;
}

int init_modules()
{
	ki = (KernelInfo*)kmalloc(sizeof(KernelInfo));
	ki->symbols = get_sym_table();
	ki->findsym = find_symbol;
	return 0;
}

int load_module(const char* name)
{
	FS_DirectoryEntry* fsi = fs_get_file_info(name);
	if (fsi)
	{
		int fh = _open(name, _O_RDONLY);
		if (fh > -1)
		{
			u8int* progbits = kmalloc(fsi->size);
			if (_read(fh, progbits, fsi->size) < fsi->size)
			{
				kfree(progbits);
				kprintf("Could not read entire module %s\n", name);
				return 0;
			}

			ModuleHeader* head = (ModuleHeader*)progbits;


			if (head->signature != 0x55444F4D)
			{
				kfree(progbits);
				kprintf("Invalid module signature in module %s\n", name);
				return 0;
			}

			Module* newmod = (Module*)kmalloc(sizeof(Module));

			newmod->next = modlist;
			newmod->name = name;
			newmod->progbits = progbits;
			newmod->size = fsi->size;
			newmod->header = head;

			modlist = newmod;

			kprintf("New module %s, init=%08x fini=%08x\n", name, newmod->header->modinit, newmod->header->modfini);

			page_module(newmod);

			newmod->header->modinit(ki);
			newmod->header->modfini(ki);

			_close(fh);
		}
	}
	return 0;
}


