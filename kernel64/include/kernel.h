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
#include "timer.h"
#include "kmalloc.h"
#include "ata.h"
#include "filesystem.h"
#include "iso9660.h"

#define assert(expr, line) if (!(expr)) { \
	kprintf("Assertion failure at %s:%s: %s", line, __FILE__,__LINE__); \
	wait_forever(); }

#endif
