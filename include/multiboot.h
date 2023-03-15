#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#define MULTIBOOT_MAGIC 0x2BADB002

#define MB_MEMORY 1
#define MB_BOOTDEV 2
#define MB_CMDLINE 4
#define MB_MODS 8
#define MB_AOUT_SYMS 16
#define MB_ELF 32
#define MB_MEMMAP 64
#define MB_DRIVEINFO 128
#define MB_CONFIGTABLE 256
#define MB_BOOTLOADERNAME 512
#define MB_APMTABLE 1024
#define MB_VIDEOINFO 2048

#define MB_MM_AVAIL 1
#define MB_MM_RESVD 2

typedef struct
{
	uint32_t size;
	uint64_t addr;
	uint64_t len;
	uint32_t type;
} MB_MemMap;

typedef struct
{
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	const char* commandline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t elf_headers_num;
	uint32_t elf_headers_size;
	uint32_t elf_headers_addr;
	uint32_t elf_headers_shndx;
	uint32_t mmap_len;
	MB_MemMap* mmap_addr;
	uint32_t drives_length;
	uint32_t drives_addr;
	uint32_t config_table;
	const char* bootloadername;
	uint32_t apm_table;
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;       
} MultiBoot; 

#endif

