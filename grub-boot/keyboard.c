#include "video.h"
#include "interrupts.h"
#include "keyboard.h"
#include "printf.h"
#include "kernel.h"
#include "io.h"

void keyboard_handler(registers_t regs);

void init_basic_keyboard()
{
	register_interrupt_handler(IRQ1, keyboard_handler);
}

static unsigned escaped = 0;
static unsigned shift_state = 0;

void keyboard_handler(registers_t regs)
{
	unsigned char new_scan_code = inb(0x60);

	if (escaped)
		new_scan_code += 256;
	switch(new_scan_code)
	{
		case 0x2a:
			shift_state = 1;
		break;
		case 0xaa:
			shift_state = 0;
		break;
		case 0xe0:
			escaped = 1;
		break;
		default:
		if ((new_scan_code & 0x80) == 0)
		{
			//new_char = (shift_state?uppercase:lowercase)[new_scan_code];
			// do something with new_char
			printf("Keyboard IRQ port val=0x%x escaped=%d shift=%d\n", new_scan_code, escaped, shift_state);
			if (escaped == 1)
				escaped = 0;
		} 
		break;
	}

}


