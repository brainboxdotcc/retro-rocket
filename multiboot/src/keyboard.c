#include "../include/video.h"
#include "../include/interrupts.h"
#include "../include/keyboard.h"
#include "../include/printf.h"
#include "../include/kernel.h"
#include "../include/io.h"

void keyboard_handler(registers_t regs);

void init_basic_keyboard()
{
	register_interrupt_handler(IRQ1, keyboard_handler);
}

static unsigned escaped = 0;
static unsigned shift_state = 0;
static unsigned ctrl_state = 0;
static unsigned alt_state = 0;

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
			//new_char = (shift_state?uppercase:lowercase)[new_scan_code];
			// do something with new_char
			printf("Key scancode=0x%x escaped=%d s=%d c=%d a=%d\n", new_scan_code, escaped, shift_state, ctrl_state, alt_state);
			if (escaped == 1)
				escaped = 0;
		} 
		break;
	}

}


