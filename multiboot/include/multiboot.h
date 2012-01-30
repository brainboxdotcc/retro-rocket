#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct
{
	u32int flags;
	u32int mem_lower;
	u32int mem_upper;
	u32int boot_device;
	const char* commandline;
	u32int mods_count;
	u32int mods_addr;
	u32int elf_headers_num;
	u32int elf_headers_size;
	u32int elf_headers_addr;
	u32int elf_headers_shndx;
	u32int mmap_len;
	u32int mmap_addr;
	u32int drives_length;
	u32int drives_addr;
	u32int config_table;
	const char* bootloadername;
	u32int apm_table;
} MultiBoot; 


#endif
