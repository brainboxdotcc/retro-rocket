#include <module.h>
#include <kernel.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <paging.h>
#include <filesystem.h>
#include <debugger.h>

Module* modlist = NULL;

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
		
			_close(fh);
		}
	}
	return 0;
}


