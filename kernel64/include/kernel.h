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
#define kprintf printf

enum bool
{
	false,
	true
};

static inline void memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
		for ( ; len != 0; len--) *temp++ = val;
}


#include "printf.h"
#include "video.h"
#include "string.h"
#include "io.h"
#include "memcpy.h"
#include "apic.h"
#include "paging.h"

/* Multiboot information structure
 * IMPORTANT NOTE: Because GRUB 0.97 is 32 bit, it loads
 * the kernel through the 'AOUT kludge' which means this
 * multiboot parameter it passes has all of its values as
 * 32 bit pointers. It may only exist within the first four
 * gigabytes of RAM (usually it is placed somewhere after
 * the loaded kernel). Be aware of its limitations when
 * accessing ANY of its pointers!
 */
typedef struct
{
	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	u32 commandline;
	u32 mods_count;
	u32 mods_addr;
	u32 elf_headers_num;
	u32 elf_headers_size;
	u32 elf_headers_addr;
	u32 elf_headers_shndx;
	u32 mmap_len;
	u32 mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	u32 bootloadername;
	u32 apm_table;
} MultiBoot; 

#endif
