#include "../include/elf.h"
#include "../include/kernel.h"
#include "../include/kmalloc.h"
#include "../include/kmalloc.h"
#include "../include/string.h"
#include "../include/printf.h"
#include "../include/filesystem.h"
#include "../include/debugger.h"

Elf32_Shdr* get_section_by_name(int fh, Elf32_Ehdr* fileheader, unsigned char* stringtable, const char* name);
Elf32_Shdr* get_sectionheader(int fh, Elf32_Ehdr* fileheader, int headernum);

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

		printf("Elf32_Ehdr\n");

		printf("e_type=%d e_machine=%d e_version=%d e_entry=%x e_phoff=%d e_shoff=%d e_flags=%d\n",
				fileheader->e_type,
				fileheader->e_machine,
				fileheader->e_version,
				fileheader->e_entry,
				fileheader->e_phoff,
				fileheader->e_shoff,
				fileheader->e_flags);
		printf("e_ehsize=%d e_phentsize=%d e_phnum=%d e_shentsize=%d e_shnum=%d e_shstrndx=%d\n",
				fileheader->e_ehsize,
				fileheader->e_phentsize,
				fileheader->e_phnum,
				fileheader->e_shentsize,
				fileheader->e_shnum,
				fileheader->e_shstrndx);
		printf("e_ident[EI_CLASS]=%d e_ident[EI_DATA]=%d\n", fileheader->e_ident[EI_CLASS],
				fileheader->e_ident[EI_DATA]);

		printf("File %s executable as x86 32 bit\n", IS_INTEL_32(fileheader) ? "is" : "is NOT");

		if (fileheader->e_type != ET_EXEC || !IS_INTEL_32(fileheader))
		{
			printf("load_elf: File not executable\n");
			kfree(fileheader);
			_close(fh);
			return 0;
		}

		Elf32_Shdr* shdr = get_sectionheader(fh, fileheader, fileheader->e_shstrndx);

		printf("String table section header\n");
		printf("Reading %d bytes of strings at offset %d\n", shdr->sh_size, shdr->sh_offset);

		stringtable = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtable, shdr->sh_size);
		kfree(shdr);

		shdr = get_section_by_name(fh, fileheader, stringtable, ".strtab");
		stringtablesym = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtablesym, shdr->sh_size);
		kfree(shdr);


		printf("Fetching %d program headers at %d\n", fileheader->e_phnum, fileheader->e_phoff);

		unsigned char* mem = (unsigned char*)kmalloc(fileheader->e_phnum * sizeof(Elf32_Phdr));
		_lseek(fh, fileheader->e_phoff, 0);
		_read(fh, mem, fileheader->e_phnum * sizeof(Elf32_Phdr));
		DumpHex(mem, fileheader->e_phnum * sizeof(Elf32_Phdr));

		Elf32_Phdr* phdr = (Elf32_Phdr*)kmalloc(sizeof(Elf32_Phdr));
		int head;
		_lseek(fh, fileheader->e_phoff, 0);
		for (head = 0; head < fileheader->e_phnum; head++)
		{
			printf("Offset: %d\n", fileheader->e_phoff + (fileheader->e_phentsize * head));
			n_read = _read(fh, phdr, fileheader->e_phentsize);
			printf("phdr: p_type=%08x p_offset=%d p_vaddr=%08x p_addr=%08x p_filesz=%d\n", phdr->p_type, phdr->p_offset,
					phdr->p_vaddr, phdr->p_addr, phdr->p_filesz);
			printf("      p_memsz=%d p_flags=%08x p_align=%d\n", phdr->p_memsz, phdr->p_flags, phdr->p_align);
		}
		kfree(phdr);


		kfree(stringtable);
		kfree(stringtablesym);
		kfree(fileheader);

		return 1;
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

