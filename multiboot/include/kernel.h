#ifndef __KERNEL__H__
#define __KERNEL__H__

#include "video.h"
#include "io.h"

#define UNUSED __attribute__ ((unused))

#define FREE_LINKED_LIST(type, start) { type n = start; for (;n->next;) { type oldn = n; n = n->next; kfree(oldn); } }
#define LINKED_LIST_COUNT(type, start, variable) { variable = 0; type n = start; for (;n->next; n = n->next, ++variable); }

#define NULL 0

#define PANIC_BANNER setforeground(current_console, COLOUR_LIGHTYELLOW); \
/*	kprintf("\n\
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
\n");*/ setforeground(current_console, COLOUR_LIGHTWHITE);\
kprintf("This is a fatal system error and your system has been halted.\n\
"); setforeground(current_console, COLOUR_LIGHTRED);

#define assert(expr, line) if (!(expr)) { \
	      kprintf("Assertion failure at %s:%s: %s", line, __FILE__,__LINE__); \
	      blitconsole(current_console); \
	      wait_forever(); }

extern console* current_console;

typedef unsigned long long	u64int;
typedef          long long	s64int;
typedef unsigned int		u32int;
typedef          int		s32int;
typedef unsigned short		u16int;
typedef          short		s16int;
typedef unsigned char		u8int;
typedef          char		s8int;

enum
{
	false = 0,
	true = 1
} bool;

void _memset(void *dest, char val, int len);


#endif
