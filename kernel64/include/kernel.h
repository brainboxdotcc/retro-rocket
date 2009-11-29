#ifndef __KERNEL_H__
#define __KERNEL_H__

typedef unsigned long u64;	// 64 bit unsigned
typedef long s64;		// 64 bit signed
typedef unsigned int u32;	// 32 bit unsigned
typedef int s32;		// 32 bit signed
typedef unsigned short u16;	// 16 bit unsigned
typedef short s16;		// 16 bit signed
typedef unsigned char u8;	// 8 bit unsigned
typedef char s8;		// 8 bit signed

#define NULL 0

enum bool
{
	false,
	true
};

#include "printf.h"
#include "video.h"
#include "string.h"
#include "io.h"
#include "memcpy.h"

// Multiboot information structure
typedef struct
{
	unsigned long flags;
	unsigned long mem_lower;
	unsigned long mem_upper;
	unsigned long boot_device;
	unsigned long commandline;
	unsigned long mods_count;
	unsigned long mods_addr;
	unsigned long elf_headers_num;
	unsigned long elf_headers_size;
	unsigned long elf_headers_addr;
	unsigned long elf_headers_shndx;
	unsigned long mmap_len;
	unsigned long mmap_addr;
} MultiBoot; 

#endif
