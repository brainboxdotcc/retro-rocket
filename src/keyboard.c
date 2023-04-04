#include <kernel.h>

static char keyboard_buffer[1024];
static int bufwriteptr = 0;
static int bufreadptr = 0;

void keyboard_handler(uint8_t isr, uint64_t errorcode, uint64_t irq);

static uint8_t escaped = 0;
static uint8_t caps_lock = 0;
static uint8_t shift_state = 0;
static uint8_t ctrl_state = 0;
static uint8_t alt_state = 0;

/* UK mappings of scan codes to characters, based in part off http://www.ee.bgu.ac.il/~microlab/MicroLab/Labs/ScanCodes.htm */

static const char keyboard_scan_map_lower[] = {0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9, 'q', 'w', 'e',
					'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0 /* CTRL */, 'a', 's', 'd', 'f', 'g', 'h',
					'j', 'k', 'l', ';', '\'', '#', 0 /* LEFT SHIFT*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
					'.', '/', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

static const char keyboard_scan_map_upper[] = {0, 27, '!', '@', '~', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9, 'Q', 'W', 'E',
					'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0 /* CTRL */, 'A', 'S', 'D', 'F', 'G', 'H',
					'J', 'K', 'L', ':', '"', '~', 0 /* LEFT SHIFT*/, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
					'>', '?', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};


void init_basic_keyboard()
{
	char devname[16];
	bufwriteptr = 0;
	bufreadptr = 0;
	make_unique_device_name("kb", devname);
	register_interrupt_handler(IRQ1, keyboard_handler);
}


// Map a keyboard scan code to an ASCII value
unsigned char translate_keycode(unsigned char scancode, uint8_t escaped, uint8_t shift_state, [[maybe_unused]] uint8_t ctrl_state, [[maybe_unused]] uint8_t alt_state)
{
	if (escaped) {
		switch (scancode) {
			/* Alt+# or Alt+3 for # symbol, a kludge for vnc/qemu */
			case 0x48:
				dprintf("UP");
			break;
			case 0x50:
				dprintf("DOWN");
			break;
			case 0x4B:
				dprintf("LEFT");
			break;
			case 0x4D:
				dprintf("RIGHT");
			break;
			default:
				dprintf("Unknown escape seq: %02x", scancode);
			break;
		}
		return 0;
	} else {
		/* Kludge for stupid vnc/qemu keymapping, alt+# for # symbol */
		if (scancode == 0x04 && alt_state) return '#';
		if (scancode > 0x53 || keyboard_scan_map_lower[scancode] == 0) {
			/* Special key */
			dprintf("Keyboard: Special key %08x not implemented yet\n", scancode);
			return 0;
		} else {
			if (caps_lock) {
				char v = (shift_state ? keyboard_scan_map_lower : keyboard_scan_map_upper)[scancode];
				if ((v < 'a' || v > 'z') && (v < 'A' || v > 'Z')) {
					v = (shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
				}
				return v;
			} else {
				return (shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
			}
		}
	}
}



void keyboard_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq)
{
	unsigned char new_scan_code = inb(0x60);

	if (escaped) {
		new_scan_code += 256;
	}
	switch(new_scan_code) {
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
			if ((new_scan_code & 0x80) == 0) {
				char x = translate_keycode(new_scan_code, escaped, shift_state, ctrl_state, alt_state);

				if (x) {
					if (bufreadptr > bufwriteptr) {
						beep(1000);
					} else {
						keyboard_buffer[bufwriteptr++] = x;
						if (bufwriteptr > 1024) {
							bufwriteptr = 0;
						}
					}
				}

				if (escaped == 1) {
					escaped = 0;
				}
			} 
		break;
	}

}


unsigned char kgetc([[maybe_unused]] console* cons)
{
	if (bufreadptr >= bufwriteptr) {
		return 255;
	}

	return keyboard_buffer[bufreadptr++];
}

