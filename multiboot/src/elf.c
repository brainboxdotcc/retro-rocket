#include "../include/elf.h"
#include "../include/kernel.h"
#include "../include/kmalloc.h"
#include "../include/kmalloc.h"
#include "../include/string.h"
#include "../include/printf.h"
#include "../include/paging.h"
#include "../include/filesystem.h"
#include "../include/debugger.h"

Elf32_Shdr* get_section_by_name(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable, const char* name);
Elf32_Shdr* get_sectionheader(int fh, Elf32_Ehdr* fileheader, int headernum);

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
		printf("load_elf: Invalid path or filename '%s'\n", path_to_file);
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
			printf("load_elf: Could not read entire Elf32_Ehdr from '%s'\n", path_to_file);
			return 0;
		}

		if (fileheader->e_ident[EI_MAG0] != ELFMAGIC0 || fileheader->e_ident[EI_MAG1] != ELFMAGIC1 ||
				fileheader->e_ident[EI_MAG2] != ELFMAGIC2 || fileheader->e_ident[EI_MAG3] != ELFMAGIC3)
		{
			kfree(fileheader);
			_close(fh);
			printf("load_elf: The file '%s' is not an ELF executable!\n", path_to_file);
			return 0;
		}

		if (fileheader->e_type != ET_EXEC || !IS_INTEL_32(fileheader))
		{
			printf("load_elf: File not executable\n");
			kfree(fileheader);
			_close(fh);
			return 0;
		}

		Elf32_Shdr* shdr = get_sectionheader(fh, fileheader, fileheader->e_shstrndx);
		stringtable = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtable, shdr->sh_size);
		kfree(shdr);
		shdr = get_section_by_name(fh, fileheader, stringtable, ".strtab");
		stringtablesym = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtablesym, shdr->sh_size);
		kfree(shdr);

		Elf32_Phdr* phdr = (Elf32_Phdr*)kmalloc(sizeof(Elf32_Phdr));
		int head, error = 0;
		u32int fd1, *eip, sz;
		page_directory_t* dir;

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
		for (head = 0; head < fileheader->e_phnum; head++)
		{
			n_read = _read(fh, phdr, fileheader->e_phentsize);
			if (n_read < fileheader->e_phentsize)
			{
				error = 1;
				printf("load_elf: Can't read whole Elf32_Phdr #%d\n", head);
				break;
			}

			switch (phdr->p_type)
			{
				case PT_LOAD:
					/* Load a section of code (.text) */
					if (phdr->p_vaddr >= 0x800000 && (phdr->p_vaddr + phdr->p_memsz < UHEAP_START))
					{
						u32int curpos = _tell(fh);
						_lseek(fh, phdr->p_offset, 0);
						sign_sect(phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz, 1, 1, current_directory);
						n_read = _read(fh, (void*)phdr->p_vaddr, phdr->p_filesz);
						printf("PT_LOAD: loaded and mapped memory from elf file\n");
						if (n_read < phdr->p_filesz)
						{
							error = 1;
							printf("load_elf: Can't read entire PT_LOAD section!\n");
						}
						_lseek(fh, curpos, 0);
					}
					else
					{
						error = 1;
						printf("load_elf: Can't map PT_LOAD section below vaddr 0x800000 or above user heap!\n");
					}
				break;
				case PT_SHLIB:
					printf("PT_SHLIB: not supported yet!\n");
				case PT_PHDR:
					/* Address of program header. This is included in the PT_LOAD sections
					 * and can be safely skipped over
					 */
				break;
				case PT_INTERP:
					printf("load_elf: Warning: PT_INTERP: not supported yet!\n");
				break;
				case PT_DYNAMIC:
					printf("load_elf: Warning: PT_DYNAMIC: not supported yet!\n");
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
			ret_addr = fileheader->e_entry;	/* Entry address of program */
			ret_esp = USTACK - 4;    /* Return Stack pointer */
		}
		else
		{
			/* OH dear. we fucked up here. There was an error in the ELF,
			 * but we already nuked our directory!
			 */
		}

		printf("Would execute from 0x%08x\n", fileheader->e_entry);

		kfree(stringtable);
		kfree(stringtablesym);
		kfree(fileheader);

		return !error;
	}
	return 0;
}

Elf32_Shdr* get_section_by_name(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable, const char* name)
{
	int sh;
	for (sh = 0; sh < fileheader->e_shnum; sh++)
	{
		Elf32_Shdr* hdr = get_sectionheader(fh, fileheader, sh);
		if (!strcmp((char*)stringtable + hdr->sh_name, name))
			return hdr;
		else
			kfree(hdr);
	}
	return NULL;
}

Elf32_Shdr* get_sectionheader(int fh, Elf32_Ehdr* fileheader, int headernum)
{
	Elf32_Shdr* shdr = (Elf32_Shdr*)kmalloc(sizeof(Elf32_Shdr));
	_lseek(fh, fileheader->e_shoff + (sizeof(Elf32_Shdr) * headernum), 0);
	int n_read = _read(fh, shdr, sizeof(Elf32_Shdr));
	return shdr;
}

