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
	u32 size;
	u64 addr;
	u64 len;
	u32 type;
} MB_MemMap;

typedef struct
{
	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	const char* commandline;
	u32 mods_count;
	u32 mods_addr;
	u32 elf_headers_num;
	u32 elf_headers_size;
	u32 elf_headers_addr;
	u32 elf_headers_shndx;
	u32 mmap_len;
	MB_MemMap* mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	const char* bootloadername;
	u32 apm_table;
	u32 vbe_control_info;
	u32 vbe_mode_info;
	u16 vbe_mode;
	u16 vbe_interface_seg;
	u16 vbe_interface_off;
	u16 vbe_interface_len;       
} MultiBoot; 

#endif

