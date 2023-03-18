#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint64_t uint64_t;
typedef uint32_t uint32_t;
typedef uint16_t uint16_t;
typedef uint8_t uint8_t;
typedef int64_t int64_t;
typedef int32_t int32_t;
typedef int16_t int16_t;
typedef int8_t int8_t;

#define kprintf printf

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
#include "printf.h"
#include "video.h"
#include "string.h"
#include "io.h"
#include "memcpy.h"
#include "apic.h"
#include "ioapic.h"
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
#include "hashmap.h"
#include "net.h"
#include "rtl8139.h"
#include "arp.h"
#include "ip.h"
#include "ethernet.h"
#include "udp.h"
#include "dhcp.h"

#define assert(expr, line) if (!(expr)) { \
	kprintf("Assertion failure at %s:%s: %s", line, __FILE__,__LINE__); \
	wait_forever(); }

