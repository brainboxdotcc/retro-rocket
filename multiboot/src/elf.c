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
		DumpHex((unsigned char*)fileheader, sizeof(Elf32_Ehdr));

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
		printf("sh_name=%d sh_type=%d sh_flags=%d sh_addr=%x sh_offset=%d sh_size=%d sh_link=%d\n",
				shdr->sh_name,
				shdr->sh_type,
				shdr->sh_flags,
				shdr->sh_addr,
				shdr->sh_offset,
				shdr->sh_size,
				shdr->sh_link);
		printf("sh_info=%d sh_addralign=%d sh_entsize=%d\n",
				shdr->sh_info,
				shdr->sh_addralign,
				shdr->sh_entsize);

		printf("Reading %d bytes of strings at offset %d\n", shdr->sh_size, shdr->sh_offset);

		stringtable = (unsigned char*)kmalloc(shdr->sh_size);
		_lseek(fh, shdr->sh_offset, 0);
		n_read = _read(fh, stringtable, sizeof(Elf32_Shdr));
		DumpHex(stringtable, shdr->sh_size);

		int sh;
		for (sh = 0; sh < fileheader->e_shnum; sh++)
		{
			Elf32_Shdr* hdr = get_sectionheader(fh, fileheader, sh);
			printf("Section %d: %s\n", sh, stringtable + hdr->sh_name);
			kfree(hdr);
		}

		kfree(stringtable);
		kfree(shdr);
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
		if (!strcmp(stringtable + hdr->sh_name, name))
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

