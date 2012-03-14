#include <elf.h>
#include <kernel.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <paging.h>
#include <filesystem.h>
#include <debugger.h>

Elf32_Shdr* get_section_by_name(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable, const char* name);
Elf32_Shdr* get_sectionheader(int fh, Elf32_Ehdr* fileheader, int headernum);
void list_named_sections(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable);

extern page_directory_t* current_directory;	/* Current page directory */
extern u32int end;				/* End of kernel */
extern u32int ret_addr;                         /* Execution address immediately after exec syscall */
extern u32int ret_esp;                          /* Stack pointer immediately after exec syscall */


u8int load_elf(const char* path_to_file)
{
	int fh = _open(path_to_file, _O_RDONLY);
	unsigned char* stringtable = NULL;
	unsigned char* stringtablesym = NULL;

	if (fh < 0)
	{
		kprintf("load_elf: Invalid path or filename '%s'\n", path_to_file);
		return 0;
	}
	else
	{
		Elf32_Ehdr* fileheader = (Elf32_Ehdr*)kmalloc(sizeof(Elf32_Ehdr));

		int n_read = _read(fh, fileheader, sizeof(Elf32_Ehdr));
		if (n_read < sizeof(Elf32_Ehdr))
		{
			kfree(fileheader);
			_close(fh);
			kprintf("load_elf: Could not read entire Elf32_Ehdr from '%s'\n", path_to_file);
			return 0;
		}

		if (fileheader->e_ident[EI_MAG0] != ELFMAGIC0 || fileheader->e_ident[EI_MAG1] != ELFMAGIC1 ||
				fileheader->e_ident[EI_MAG2] != ELFMAGIC2 || fileheader->e_ident[EI_MAG3] != ELFMAGIC3)
		{
			kfree(fileheader);
			_close(fh);
			kprintf("load_elf: The file '%s' is not an ELF executable!\n", path_to_file);
			return 0;
		}

		if (fileheader->e_type != ET_DYN || !IS_INTEL_32(fileheader))
		{
			kprintf("load_elf: File not executable %02x\n", fileheader->e_type);
			kfree(fileheader);
			_close(fh);
			return 0;
		}

		Elf32_Shdr* shdr = get_sectionheader(fh, fileheader, fileheader->e_shstrndx);
		stringtable = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtable, shdr->sh_size);
		kfree(shdr);
		shdr = get_section_by_name(fh, fileheader, stringtable, ".dynstr");
		stringtablesym = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtablesym, shdr->sh_size);
		kfree(shdr);

		list_named_sections(fh, fileheader, stringtable);

		Elf32_Phdr* phdr = (Elf32_Phdr*)kmalloc(sizeof(Elf32_Phdr));
		int head, error = 0;

		u32int* loadaddrs = kmalloc(fileheader->e_phnum * sizeof(u32int));
		u32int* virtaddrs = kmalloc(fileheader->e_phnum * sizeof(u32int));
		u32int* loadsizes = kmalloc(fileheader->e_phnum * sizeof(u32int));
		_memset(loadaddrs, 0, fileheader->e_phnum * sizeof(u32int));
		_memset(virtaddrs, 0, fileheader->e_phnum * sizeof(u32int));
		_memset(loadsizes, 0, fileheader->e_phnum * sizeof(u32int));

		/*
		XXX: We really REALLY need to validate all the elf sections
		     BEFORE we do this, otherwise we leave ourselves in a right
		     mess if we encounter an unparseable section!

		dir = current_directory;
		proc_current->dir = init_procdir();
		switch_page_directory(proc_current->dir);
		kill_directory(dir);
		*/

		_lseek(fh, fileheader->e_phoff, 0);
		kprintf("Headers: %d\n", fileheader->e_phnum);
		for (head = 0; head < fileheader->e_phnum; head++)
		{
			n_read = _read(fh, phdr, fileheader->e_phentsize);

			if (n_read < fileheader->e_phentsize)
			{
				error = 1;
				kprintf("load_elf: Can't read whole Elf32_Phdr #%d\n", head);
				break;
			}
			//kprintf("Program header %d of %d type %02x\n", head, fileheader->e_phnum, phdr->p_type);

			switch (phdr->p_type)
			{
				case PT_LOAD:
					/* Load a section of code (.text) */
					//if (phdr->p_vaddr >= UPROGSTART && (phdr->p_vaddr + phdr->p_memsz < UHEAP_START))
					{
						u32int curpos2 = _tell(fh);
						_lseek(fh, phdr->p_offset, 0);
						//sign_sect(phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz, 1, 1, current_directory);
						/* The ELF spec says the memory must be zeroed, and to make life easy for ourselves we
						 * always make sure this is page-aligned.
						*/
						u8int* alloc = kmalloc_ext(phdr->p_memsz, 1, 0);

						loadaddrs[head] = alloc;
						virtaddrs[head] = phdr->p_vaddr;
						loadsizes[head] = phdr->p_memsz;

						// save phdr->p_vaddr and alloc for later relocation.

						_memset(alloc, 0, phdr->p_memsz);
						//
						// NOTE: Here, the actual load address compiled for is phdr->p_vaddr,
						// this is what we are relocating to when we fixup.
						//
						n_read = _read(fh, alloc, phdr->p_filesz);
						_lseek(fh, curpos2, 0);
						if (n_read < phdr->p_filesz)
						{
							error = 1;
							kprintf("load_elf: Can't read entire PT_LOAD section!\n");
							break;
						}
						kprintf("PT_LOAD: loaded and mapped %d bytes memory from elf file to 0x%08x\n", loadsizes[head], loadaddrs[head]);
					}
					//else
					//{
						//error = 1;
						//kprintf("load_elf: Can't map PT_LOAD section below vaddr 0x%08x or above user heap!\n", UPROGSTART);
					//}
				break;
				case PT_SHLIB:
					//kprintf("PT_SHLIB: not supported yet!\n");
				case PT_PHDR:
					/* Address of program header. This is included in the PT_LOAD sections
					 * and can be safely skipped over
					 */
				break;
				case PT_INTERP:
				{
					u32int curpos = _tell(fh);

					_lseek(fh, phdr->p_offset, 0);
					unsigned char* interpreter = (unsigned char*)kmalloc(phdr->p_memsz);
					n_read = _read(fh, interpreter, phdr->p_filesz);
					if (n_read < phdr->p_filesz)
					{
						error = 1;
						//kprintf("load_elf: Can't read entire interpreter name!\n");
					}
					//kprintf("load_elf: File %s specifies an interpreter '%s', but this is not supported.\n", path_to_file, interpreter);
					_lseek(fh, curpos, 0);
					kfree(interpreter);
				}
				break;
				case PT_DYNAMIC:
				{
					//kprintf("load_elf: Warning: PT_DYNAMIC: not supported yet!\n");
					u32int curpos = _tell(fh);
					_lseek(fh, phdr->p_offset, 0);
					u8int* dynamic = (u8int*)kmalloc(phdr->p_memsz);
					_read(fh, dynamic, phdr->p_filesz);
					//DumpHex(dynamic, phdr->p_memsz);
					kfree(dynamic);
					_lseek(fh, curpos, 0);
				}
				break;
				case PT_NULL:
				case PT_GNU_STACK:
				case PT_PAX_FLAGS:
					/* Nothing done for these */
				break;
			}

			if (error)
				break;
		}
		kfree(phdr);

		if (!error)
		{
			//ret_addr = fileheader->e_entry;	/* Entry address of program */
			//ret_esp = USTACK - 4;    /* Return Stack pointer */
		}
		else
		{
			/* OH dear. we fucked up here. There was an error in the ELF,
			 * but we already nuked our directory!
			 */
		}

		Elf32_Shdr* reloc_hdr = get_section_by_name(fh, fileheader, stringtable, ".text.rel");
		_lseek(fh, reloc_hdr->sh_offset, 0);
		u8int* relocations = (u8int*)kmalloc(reloc_hdr->sh_size);
		_read(fh, relocations, reloc_hdr->sh_size);
		DumpHex(relocations, reloc_hdr->sh_size);



		//kprintf("Would execute from 0x%08x\n", fileheader->e_entry);

		_close(fh);
		kfree(relocations);
		kfree(stringtable);
		kfree(stringtablesym);
		kfree(fileheader);

		return !error;
	}
	return 0;
}

void list_named_sections(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable)
{
	int sh;
	for (sh = 0; sh < fileheader->e_shnum; sh++)
	{
		Elf32_Shdr* hdr = get_sectionheader(fh, fileheader, sh);
		if (*(stringtable + hdr->sh_name))
			kprintf("%d: %s ", sh, stringtable + hdr->sh_name);
		kfree(hdr);
	}
	kprintf("\n");
}

Elf32_Shdr* get_section_by_name(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable, const char* name)
{
	int sh;
	for (sh = 0; sh < fileheader->e_shnum; sh++)
	{
		Elf32_Shdr* hdr = get_sectionheader(fh, fileheader, sh);
		if (!strcmp((char*)stringtable + hdr->sh_name, name))
		{
			//kprintf("Section %d = %s\n", sh, name);
			return hdr;
		}
		else
			kfree(hdr);
	}
	return NULL;
}

Elf32_Shdr* get_sectionheader(int fh, Elf32_Ehdr* fileheader, int headernum)
{
	//kprintf("get_sectionheader %d max=%d ", headernum, fileheader->e_shnum);
	if (headernum < 0 || headernum > fileheader->e_shnum)
		return NULL;

	Elf32_Shdr* shdr = (Elf32_Shdr*)kmalloc(sizeof(Elf32_Shdr));
	if (_lseek(fh, fileheader->e_shoff + (sizeof(Elf32_Shdr) * headernum), 0) == -1)
	{
		//kprintf("Bad offset %d\n",fileheader->e_shoff + (sizeof(Elf32_Shdr) * headernum));
		kfree(shdr);
		return NULL;
	}
	int n_read = _read(fh, shdr, sizeof(Elf32_Shdr));
	if (n_read < sizeof(Elf32_Shdr))
	{
		//kprintf("Didnt read everything\n");
		kfree(shdr);
		return NULL;
	}
	//kprintf("Name=%d\n", shdr->sh_name);
	return shdr;
}

