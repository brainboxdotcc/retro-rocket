#include <kernel.h>

static char keyboard_buffer[1024];
static int bufwriteptr = 0;
static int bufreadptr = 0;

void keyboard_handler(u8 isr, u64 errorcode, u64 irq);

static u8 escaped = 0;
static u8 caps_lock = 0;
static u8 shift_state = 0;
static u8 ctrl_state = 0;
static u8 alt_state = 0;

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
	bufwriteptr = 0;
	bufreadptr = 0;
	register_interrupt_handler(IRQ1, keyboard_handler);
}


// Map a keyboard scan code to an ASCII value
unsigned char translate_keycode(unsigned char scancode, u8 escaped, u8 shift_state, u8 ctrl_state, u8 alt_state)
{
	if (escaped)
	{
		switch (scancode)
		{
			case 0x48:
				kprintf("UP");
			break;
			case 0x50:
				kprintf("DOWN");
			break;
			case 0x4B:
				kprintf("LEFT");
			break;
			case 0x4D:
				kprintf("RIGHT");
			break;
			default:
				kprintf("Unknown escape seq: %02x", scancode);
			break;
		}
		return 0;
	}
	else
	{
		if (scancode > 0x53 || keyboard_scan_map_lower[scancode] == 0)
		{
			/* Special key */
			kprintf("Keyboard: Special key %08x not implemented yet\n", scancode);
			return 0;
		}
		else
		{
			if (caps_lock)
			{
				char v = (shift_state ? keyboard_scan_map_lower : keyboard_scan_map_upper)[scancode];
				if (v < 'a' || v > 'z')
					if (v < 'A' || v > 'Z')
						v = (shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
	
				return v;
			}
			else
				return (shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
		}
	}
}



void keyboard_handler(u8 isr, u64 errorcode, u64 irq)
{
	unsigned char new_scan_code = inb(0x60);

	//kprintf("%02x", new_scan_code);

	if (escaped)
		new_scan_code += 256;
	switch(new_scan_code)
	{
		case 0x2a:
		case 0x36:
			shift_state = 1;
		break;
		case 0x3A:
			caps_lock = 1;
		break;
		case 0xBA:
			caps_lock = 0;
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
				if (bufreadptr > bufwriteptr)
				{
					beep(1000);
				}
				else
				{
					keyboard_buffer[bufwriteptr++] = x;
					if (bufwriteptr > 1024)
						bufwriteptr = 0;
				}
			}

			if (escaped == 1)
				escaped = 0;
		} 
		break;
	}

}


unsigned char kgetc(console* cons)
{
	if (bufreadptr >= bufwriteptr)
		return 255;

	return keyboard_buffer[bufreadptr++];

}

