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
	u32int size;
	u32int addrlow;
	u32int addrhigh;
	u32int lenlow;
	u32int lenhigh;
	u32int type;
} MB_MemMap;

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
	MB_MemMap* mmap_addr;
	u32int drives_length;
	u32int drives_addr;
	u32int config_table;
	const char* bootloadername;
	u32int apm_table;
	u32int vbe_control_info;
	u32int vbe_mode_info;
	u16int vbe_mode;
	u16int vbe_interface_seg;
	u16int vbe_interface_off;
	u16int vbe_interface_len;       
} MultiBoot; 

#endif
