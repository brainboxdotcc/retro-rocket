#ifndef __KERNEL__H__
#define __KERNEL__H__

#include "video.h"

#define ASSERT(x) 

#define FREE_LINKED_LIST(type, start) { type n = start; for (;n->next;) { type oldn = n; n = n->next; kfree(oldn); } }
#define LINKED_LIST_COUNT(type, start, variable) { variable = 0; type n = start; for (;n->next; n = n->next, ++variable); }

#define NULL 0
extern console* current_console;

typedef unsigned int   u32int;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

void _memset(void *dest, char val, int len);

#endif
