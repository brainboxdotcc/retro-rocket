#ifndef __ELF_H__
#define __ELF_H__

#include "kernel.h"

#define UPROGSTART 0x1000000

typedef u32int Elf32_Addr;
typedef u16int Elf32_Half;
typedef u32int Elf32_Off;
typedef s32int Elf32_Sword;
typedef u32int Elf32_Word;

#define EI_NIDENT     16

typedef struct
{
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;	/* Virtual address of start of executable */
	Elf32_Off e_phoff;	/* Program header table offset */
	Elf32_Off e_shoff;	/* Section header table offset */
	Elf32_Word e_flags;	/* Processor specific flags */
	Elf32_Half e_ehsize;	/* ELF header size */
	Elf32_Half e_phentsize;	/* Size of one program header entry */
	Elf32_Half e_phnum;	/* Number of entries in program header */
	Elf32_Half e_shentsize;	/* Size of one section header */
	Elf32_Half e_shnum;	/* Number of entries in section header table */
	Elf32_Half e_shstrndx;	/* Index of string table in section header */
} __attribute__((__packed__)) Elf32_Ehdr;

	/* The string table is a list of null terminated strings. */

	/* Values for Elf32_Ehdr::e_type */
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

	/* Values for Elf32_Ehdr::e_machine */
#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8

	/* Values for Elf32_Ehdr::e_version */
#define EV_NONE 0
#define EV_CURRENT 1

/* Offset values into Elf32_Ehdr::e_ident */
enum IdentValues
{
	EI_MAG0, EI_MAG1, EI_MAG2, EI_MAG3, EI_CLASS, EI_DATA, EI_VERSION, EI_PAD
};

#define ELFMAGIC0 0x7F
#define ELFMAGIC1 'E'
#define ELFMAGIC2 'L'
#define ELFMAGIC3 'F'

	/* Values for e_ident[EI_CLASS] */
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

	/* Values for e_ident[EI_DATA] */
#define ELFDATANONE 0
#define ELFDATA2LSB 1	/* Little-endian */
#define ELFDATA2MSB 2	/* Big-endian */

/* Validates if this is a 32 bit x86 executable */
#define IS_INTEL_32(header) (header->e_ident[EI_CLASS] == ELFCLASS32 && header->e_ident[EI_DATA] == ELFDATA2LSB && header->e_machine == EM_386 && header->e_flags == 0)

/* Validates if this is a 64 bit x86_64 executable */
#define IS_INTEL_64(header) (header->e_ident[EI_CLASS] == ELFCLASS64 && header->e_ident[EI_DATA] == ELFDATA2LSB && header->e_machine == EM_386 && header->e_flags == 0)

	/* Special section indexes */
#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

typedef struct
{
	Elf32_Word sh_name;	/* Index into string table section */
	Elf32_Word sh_type;	/* Section type */
	Elf32_Word sh_flags;	/* Section flags */
	Elf32_Addr sh_addr;	/* Section load address or 0 */
	Elf32_Off  sh_offset;	/* Section offset, from beginning of file to the data */
	Elf32_Word sh_size;	/* Section size in bytes */
	Elf32_Word sh_link;	/* Link to header table index */
	Elf32_Word sh_info;	/* Extra information */
	Elf32_Word sh_addralign;/* Alignment constraints, or 0/1 for no constraints */
	Elf32_Word sh_entsize;	/* Size of each entry in the data in bytes */
} __attribute__((__packed__)) Elf32_Shdr;

	/* Values for Elf32_Shdr::sh_type */
#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

	/* Values for Elf32_Shdr::sh_flags */
#define SHF_WRITE		0x01		/* Contains writeable data */
#define SHF_ALLOC		0x02		/* Should be loaded during process execution */
#define SHF_EXECINSTR		0x04		/* Contains executable data */
#define SHF_MASKPROC		0xf0000000	/* Processor specific values mask */

/* Relocation entry info */
typedef struct
{
	Elf32_Addr  r_offset;
	Elf32_Word  r_info;
} __attribute__((__packed__)) Elf32_Rel;

typedef struct
{
	Elf32_Addr  r_offset;	/* Location of relocation, virtual address */
	Elf32_Word  r_info;	/* Symbol table index and type of relocation */
	Elf32_Sword r_addend;
} __attribute__((__packed__)) Elf32_Rela;

#define ELF32_R_SYM(i)    ((i)>>8)
#define ELF32_R_TYPE(i)   ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

/* Symbol table entry */
typedef struct
{
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;
} __attribute__((__packed__)) Elf32_Sym;

#define ELF32_ST_BIND(i)	((i) >> 4)
#define ELF32_ST_TYPE(i)	((i) & 0x0F)
#define ELF32_ST_INFO(b, t)	(((b) << 4) + ((t) & 0x0F))

/* Program header */
typedef struct
{
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_addr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
}  Elf32_Phdr;

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

/* Not officially part of the specification, but used by many open
 * source operating systems
 */
#define PT_LOOS			0x60000000		/* OS-specific start */
#define PT_HIOS			0x6fffffff		/* OS-specific end */

/* These are OS specific craq that linux throws into the elf binary.
 * We don't actually interpret these sections but we do log them
 * and make the code aware of their existence.
 */
#define PT_GNU_EH_FRAME		0x6474e550		/* Used for C++ constructors somehow */
#define PT_SUNW_EH_FRAME	PT_GNU_EH_FRAME
#define PT_GNU_STACK		(PT_LOOS + 0x474E551)	/* GNU craq, PAX executable stack flags */
#define PT_GNU_RELRO		(PT_LOOS + 0x474E552)	/* GNU craq, PAX readonly relocated locations */
#define PT_PAX_FLAGS		(PT_LOOS + 0x5041580)	/* GNU craq, PAX flags */


#endif

