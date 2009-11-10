#include "../include/video.h"
#include "../include/interrupts.h"
#include "../include/keyboard.h"
#include "../include/printf.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"

void keyboard_handler(registers_t regs);
static int ringbuffer_truncate(ringbuffer * rb, unsigned long ulong);

void init_basic_keyboard()
{
	register_interrupt_handler(IRQ1, keyboard_handler);
}

static unsigned escaped = 0;
static unsigned shift_state = 0;
static unsigned ctrl_state = 0;
static unsigned alt_state = 0;

#define ulong_to_offset(x, ulong) ((((ulong)-(x)->start) < (x)->size) ? ((ulong)-(x)->start) : ((ulong)-(x)->start) - (x)->size);


void keyboard_handler(registers_t regs)
{
	unsigned char new_scan_code = inb(0x60);

	//printf("Raw scan: %x\n", new_scan_code);
	if (escaped)
		new_scan_code += 256;
	switch(new_scan_code)
	{
		case 0x2a:
		case 0x36:
			shift_state = 1;
		break;
		case 0xaa:
		case 0xb6:
			shift_state = 0;
		break;
		case 0x1d:
			ctrl_state = 1;
		break;
		case 0x9d:
			ctrl_state = 0;
		break;
		case 0x38:
			alt_state = 1;
		break;
		case 0xb8:
			alt_state = 0;
		break;
		case 0xe0:
			escaped = 1;
		break;
		default:
		if ((new_scan_code & 0x80) == 0)
		{
			printf("Key scancode=0x%x escaped=%d s=%d c=%d a=%d\n", new_scan_code, escaped, shift_state, ctrl_state, alt_state);
			if (escaped == 1)
				escaped = 0;
		} 
		break;
	}

}


ringbuffer * rb_create(u32int size, unsigned long initialOffset)
{
	ringbuffer * ret = (ringbuffer*)kmalloc(sizeof(ringbuffer));
	ret->buf = (char*)kmalloc(size);
	ret->size = size;
	ret->start = initialOffset % size;
	ret->end = initialOffset % size;
	return ret;
}

void rb_free(ringbuffer * rb)
{
	kfree(rb->buf);
	kfree(rb);
}

static void rb_read(char * dest, ringbuffer * rb, unsigned long ulong, u32int size)
{
	unsigned long offset = ulong_to_offset(rb, ulong);
	if(offset + size < rb->size) {
		memcpy(dest, &(rb->buf[offset]), size);
	}
	else
	{
		int firstPieceLength = rb->size - offset;
		int secondPieceLength = size - firstPieceLength;
		memcpy(dest, &(rb->buf[offset]), firstPieceLength);
		memcpy(dest + firstPieceLength, &(rb->buf[0]), secondPieceLength);
	}
}

static void rb_write(ringbuffer * rb, char *src, unsigned long ulong, u32int size)
{
	int offset = ulong_to_offset(rb, ulong);
	if(offset + size < rb->size)
	{
		memcpy(&(rb->buf[offset]), src, size);
	} 
	else
	{
		int firstPieceLength = rb->size - offset;
		int secondPieceLength = size - firstPieceLength;
		memcpy(&(rb->buf[offset]), src, firstPieceLength);
		memcpy(&(rb->buf[0]), src + firstPieceLength, secondPieceLength);
	}
}

int rb_appenddata(ringbuffer * rb, char * dat, u32int size)
{
	if(size > rb->size)
		return -1;			 // Too large for buffer

	if(rb->size < (rb->end-rb->start) + size)
		return -2;			 // No more free space

	rb_write(rb, dat, rb->end, size);
	rb->end += size;

	return 0;

}

unsigned long rb_getappendpos(ringbuffer * rb)
{
	return rb->end;
}

unsigned long rb_getreadpos(ringbuffer * rb)
{
	return rb->start;
}

int rb_readdata(char * buf, ringbuffer * rb, u32int size)
{
	if (size > rb->size)
		return -1;			 // Request for chunk larger than entire ringbuffer

	if (rb->start + size > rb->end)
		return -2;

	unsigned long n = ulong_to_offset(rb, rb->start);
	rb_read(buf, rb, n, size);

	return ringbuffer_truncate(rb, rb->start + size);
}

static int ringbuffer_truncate(ringbuffer * rb, unsigned long ulong)
{
	rb->start = ulong;
	return 0;
}

