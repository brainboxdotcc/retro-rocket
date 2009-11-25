#include "../include/video.h"
#include "../include/interrupts.h"
#include "../include/keyboard.h"
#include "../include/printf.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"
#include "../include/timer.h"

static ringbuffer* keyboard_buffer;

void keyboard_handler(registers_t* regs);
static int ringbuffer_truncate(ringbuffer * rb, unsigned long ulong);

static u8int escaped = 0;
static u8int shift_state = 0;
static u8int ctrl_state = 0;
static u8int alt_state = 0;

#define ulong_to_offset(x, ulong) ((((ulong)-(x)->start) < (x)->size) ? ((ulong)-(x)->start) : ((ulong)-(x)->start) - (x)->size);

/* UK mappings of scan codes to characters, based in part off http://www.ee.bgu.ac.il/~microlab/MicroLab/Labs/ScanCodes.htm */

static const char keyboard_scan_map_lower[] = {0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9, 'q', 'w', 'e',
					'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0 /* CTRL */, 'a', 's', 'd', 'f', 'g', 'h',
					'j', 'k', 'l', ';', '\'', '#', 0 /* LEFT SHIFT*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
					'.', '/', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

static const char keyboard_scan_map_upper[] = {0, 27, '!', '"', '?', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9, 'Q', 'W', 'E',
					'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0 /* CTRL */, 'A', 'S', 'D', 'F', 'G', 'H',
					'J', 'K', 'L', ':', '@', '~', 0 /* LEFT SHIFT*/, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
					'>', '?', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};


void init_basic_keyboard()
{
	keyboard_buffer = rb_create(128, 0);
	register_interrupt_handler(IRQ1, keyboard_handler);
}


// Map a keyboard scan code to an ASCII value
unsigned char translate_keycode(unsigned char scancode, u8int escaped, u8int shift_state, u8int ctrl_state, u8int alt_state)
{
	if (scancode > 0x53 || keyboard_scan_map_lower[scancode] == 0)
	{
		/* Special key */
		printf("Keyboard: Special key not implemented yet\n");
		return 0;
	}
	else
		return (shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
}



void keyboard_handler(registers_t* regs)
{
	unsigned char new_scan_code = inb(0x60);

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
			char x = translate_keycode(new_scan_code, escaped, shift_state, ctrl_state, alt_state);

			if (x)
			{
				if (rb_appenddata(keyboard_buffer, &x, 1) != 0)
				{
					// Error inserting keypress into the buffer, emit a beep
					beep(1000);
				}
				else
				{
					printf("Inserted key '%c'\n", x);
				}
			}

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

