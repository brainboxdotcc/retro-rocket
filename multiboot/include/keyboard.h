#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "kernel.h"

typedef struct
{
	char* buf;
	unsigned int size;
	unsigned long start;
	unsigned long end;
} ringbuffer;

void init_basic_keyboard();

ringbuffer * rb_create(u32int size, unsigned long initialOffset);
void rb_free(ringbuffer * rb);
int rb_appenddata(ringbuffer * rb, char * dat, u32int size);
int rb_readdata(char * buf, ringbuffer * rb, u32int size);
unsigned long rb_getappendpos(ringbuffer * rb);
unsigned long rb_getreadpos(ringbuffer * rb);

#endif
