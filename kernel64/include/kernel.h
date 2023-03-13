#pragma once

typedef unsigned long long u64;	// 64 bit unsigned
typedef long long s64;		// 64 bit signed
typedef unsigned int u32;	// 32 bit unsigned
typedef int s32;		// 32 bit signed
typedef unsigned short u16;	// 16 bit unsigned
typedef short s16;		// 16 bit signed
typedef unsigned char u8;	// 8 bit unsigned
typedef char s8;		// 8 bit signed

typedef u64 uint64_t;
typedef u32 uint32_t;
typedef u16 uint16_t;
typedef u8 uint8_t;

typedef s64 int64_t;
typedef s32 int32_t;
typedef s16 int16_t;
typedef s8 int8_t;

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

#define PANIC_BANNER setforeground(current_console, COLOUR_LIGHTYELLOW); \
      kprintf("\n\
              ___  _____ \n\
            .'/,-Y\"     \"~-. \n\
            l.Y             ^. \n\
            /\\               _\\_      \"Yaaaaah! oh-my-GOD!\n\
           i            ___/\"   \"\\     We're trapped inside some\n\
           |          /\"   \"\\   o !    computer-dimension-\n\
           l         ]     o !__./     something-or-another!\"\n\
            \\ _  _    \\.___./    \"~\\ \n\
             X \\/ \\            ___./ \n\
            ( \\ ___.   _..--~~\"   ~`-. \n\
             ` Z,--   /               \\ \n\
               \\__.  (   /       ______) \n\
                 \\   l  /-----~~\" / \n\
                  Y   \\          /\n\
                  |    \"x______.^ \n\
                  |           \\ \n\
\n"); setforeground(current_console, COLOUR_LIGHTWHITE);\
kprintf("This is a fatal system error and your system has been halted.\n\
"); setforeground(current_console, COLOUR_LIGHTRED);

#include "limine.h"
#include "idt.h"
#include "spinlock.h"
#include "hydrogen.h"
#include "printf.h"
#include "video.h"
#include "string.h"
#include "io.h"
#include "memcpy.h"
#include "apic.h"
#include "ioapic.h"
#include "paging.h"
#include "timer.h"
#include "kmalloc.h"
#include "interrupt.h"
#include "ata.h"
#include "filesystem.h"
#include "iso9660.h"
#include "devfs.h"
#include "fat32.h"
#include "debugger.h"
#include "errorhandler.h"
#include "keyboard.h"
#include "input.h"
#include "pci.h"
#include "lapic_timer.h"
#include "ubasic.h"
#include "tokenizer.h"
#include "taskswitch.h"
#include "acpi.h"

#define assert(expr, line) if (!(expr)) { \
	kprintf("Assertion failure at %s:%s: %s", line, __FILE__,__LINE__); \
	wait_forever(); }

