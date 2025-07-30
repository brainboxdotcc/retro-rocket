#include <kernel.h>

void keyboard_handler(uint8_t isr, uint64_t errorcode, uint64_t irq, void* opaque);
unsigned char translate_keycode(unsigned char scancode, uint8_t escaped, uint8_t shift_state, [[maybe_unused]] uint8_t ctrl_state, [[maybe_unused]] uint8_t alt_state);
static void push_to_buffer(char x);

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static size_t buffer_write_ptr = 0;
static size_t buffer_read_ptr = 0;
static bool escaped = false;
static bool caps_lock = false;
static bool shift_state = false;
static bool ctrl_state = false;
static bool alt_state = false;

static struct key_state key_states[256] = { 0 };

/* UK mappings of scan codes to characters, based in part off http://www.ee.bgu.ac.il/~microlab/MicroLab/Labs/ScanCodes.htm */

static char keyboard_scan_map_lower[MAX_SCANCODE] = {0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9, 'q', 'w', 'e',
					'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0 /* CTRL */, 'a', 's', 'd', 'f', 'g', 'h',
					'j', 'k', 'l', ';', '\'', '#', 0 /* LEFT SHIFT*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
					'.', '/', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

static char keyboard_scan_map_upper[MAX_SCANCODE] = {0, 27, '!', '@', '~', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9, 'Q', 'W', 'E',
					'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0 /* CTRL */, 'A', 'S', 'D', 'F', 'G', 'H',
					'J', 'K', 'L', ':', '"', '~', 0 /* LEFT SHIFT*/, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
					'>', '?', 0 /* RIGHT SHIFT */, 0 /* PRT SCR */, 0 /* ALT */, ' ', 0 /* CAPS LOCK */, 0 /* F1 */,
					 0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0 /* F6 */, 0 /* F7 */, 0 /* F8 */, 0 /* F9 */,
					 0 /* F10 */, 0 /* NUMLOCK */, 0 /* SCROLL LOCK */, 0 /* HOME */, 0 /* UP */, 0 /* PGUP */,
					'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};


/**
 * @brief Parse one token (hex or literal) from keymap text
 *
 * Supported formats:
 *   a       -> character 'a'
 *   &41     -> hex value (65 = 'A')
 *
 * Advances *p to the end of the token.
 */
static char parse_token(const char** p) {
	const char* s = *p;
	char c = 0;

	if (*s == '&') {     /* hex literal, e.g. &41 = 'A' */
		s++;
		const char* start = s;
		while ((*s >= '0' && *s <= '9') ||
		       (*s >= 'A' && *s <= 'F') ||
		       (*s >= 'a' && *s <= 'f')) {
			s++;
		}
		char buf[16];
		size_t len = (size_t)(s - start);
		if (len >= sizeof(buf)) len = sizeof(buf) - 1;
		for (size_t i = 0; i < len; i++) buf[i] = start[i];
		buf[len] = 0;
		c = (char)atoll(buf, 16);
	} else {             /* bare character literal */
		c = *s++;
	}

	/* skip trailing non-token chars (up to whitespace/newline) */
	while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') s++;
	*p = s;
	return c;
}

/**
 * @brief Load a keymap from a null-terminated text string
 *
 * Format:
 *   &1E a A     # scancode &1E produces 'a' and 'A'
 *   &02 1 !     # scancode &02 produces '1' and '!'
 *   &1C &0D     # scancode &1C is Enter (hex 0D)
 *
 * - Lines starting with '#' are comments.
 * - First field must be scancode prefixed with & (hexadecimal).
 * - Two tokens may follow: normal (unshifted) and shifted.
 * - If only one token is given, shifted = normal.
 */
void load_keymap_from_string(const char* text) {
	const char* p = text;

	while (*p) {
		/* skip leading whitespace and carriage returns */
		while (*p == ' ' || *p == '\t' || *p == '\r') p++;
		if (*p == 0) break;

		/* skip comment lines */
		if (*p == '#') {
			while (*p && *p != '\n') p++;
			continue;
		}

		/* expect scancode starting with & */
		if (*p != '&') {
			/* invalid line: skip until next line */
			while (*p && *p != '\n') p++;
			if (*p == '\n') p++;
			continue;
		}
		p++; /* skip '&' */

		/* parse hex scancode */
		const char* start = p;
		while ((*p >= '0' && *p <= '9') ||
		       (*p >= 'A' && *p <= 'F') ||
		       (*p >= 'a' && *p <= 'f')) {
			p++;
		}

		char buf[16];
		size_t len = (size_t)(p - start);
		if (len >= sizeof(buf)) len = sizeof(buf) - 1;
		for (size_t i = 0; i < len; i++) buf[i] = start[i];
		buf[len] = 0;

		unsigned int scancode = (unsigned int)atoll(buf, 16);

		/* parse up to 2 tokens (normal + shift) */
		char normal = 0, shift = 0;
		int field = 0;
		while (*p && *p != '\n') {
			/* skip whitespace */
			while (*p == ' ' || *p == '\t') p++;
			if (*p == 0 || *p == '\n') break;

			char tok = parse_token(&p);
			if (field == 0) normal = tok;
			else if (field == 1) shift = tok;
			field++;
		}

		/* if only one token, make shift=normal */
		if (field == 1) shift = normal;

		/* store mapping if valid */
		if (scancode < MAX_SCANCODE) {
			keyboard_scan_map_lower[scancode] = normal;
			keyboard_scan_map_upper[scancode] = shift;
		}

		/* consume newline if present */
		if (*p == '\n') p++;
	}
}

void keyboard_repeat_tick()
{
	for (int i = 0; i < 256; i++) {
		if (key_states[i].down) {
			key_states[i].ticks_held++;
			if (key_states[i].ticks_held == 1) {
				// first press was already delivered in handler
				continue;
			}
			if (key_states[i].ticks_held > REPEAT_DELAY_TICKS &&
			    (key_states[i].ticks_held - REPEAT_DELAY_TICKS) % REPEAT_RATE_TICKS == 0) {
				unsigned char ch = translate_keycode(i, escaped, shift_state, ctrl_state, alt_state);
				push_to_buffer(ch);
			}
		}
	}
}


void init_keyboard()
{
	char devname[16];
	buffer_write_ptr = 0;
	buffer_read_ptr = 0;
	make_unique_device_name("kb", devname);
	register_interrupt_handler(IRQ1, keyboard_handler, dev_zero, NULL);
	proc_register_idle(keyboard_repeat_tick, IDLE_BACKGROUND);
}


// Map a keyboard scan code to an ASCII value
unsigned char translate_keycode(unsigned char scancode, uint8_t _escaped, uint8_t _shift_state, [[maybe_unused]] uint8_t _ctrl_state, [[maybe_unused]] uint8_t _alt_state)
{
	if (_escaped) {
		switch (scancode) {
			/* Alt+# or Alt+3 for # symbol, a kludge for vnc/qemu */
			case 0x48:
				return KEY_UP;
			break;
			case 0x50:
				return KEY_DOWN;
			break;
			case 0x4B:
				return KEY_LEFT;
			break;
			case 0x4D:
				return KEY_RIGHT;
			break;
			case 0x47:
				return KEY_HOME;
			break;
			case 0x4F:
				return KEY_END;
			break;
			case 0x49:
				return KEY_PAGEUP;
			break;
			case 0x51:
				return KEY_PAGEDOWN;
			break;
			case 0x52:
				return KEY_INS;
			break;
			case 0x53:
				return KEY_DEL;
			break;
			default: {
				escaped = false;
			}
			break;
		}
	}
	if (_escaped) {
		return 0;
	}
	/* Kludge for stupid vnc/qemu keymapping, alt+# for # symbol */
	if (scancode == 0x04 && alt_state) {
		return '#';
	}
	if (scancode > 0x53 || keyboard_scan_map_lower[scancode] == 0) {
		/* Special key */
		dprintf("Keyboard: Special key %08x not implemented yet\n", scancode);
		return 0;
	} else {
		if (caps_lock) {
			char v = (_shift_state ? keyboard_scan_map_lower : keyboard_scan_map_upper)[scancode];
			if ((v < 'a' || v > 'z') && (v < 'A' || v > 'Z')) {
				v = (_shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
			}
			return v;
		} else {
			return (_shift_state ? keyboard_scan_map_upper : keyboard_scan_map_lower)[scancode];
		}
	}
}

bool ctrl_held()
{
	return ctrl_state;
}

bool shift_held()
{
	return shift_state;
}

bool alt_held()
{
	return alt_state;
}

bool caps_lock_on()
{
	return caps_lock;
}

static void push_to_buffer(char x) {
	if (buffer_read_ptr > buffer_write_ptr) {
		beep(1000); // buffer overflow signal
	} else {
		keyboard_buffer[buffer_write_ptr++] = x;
		if (buffer_write_ptr >= sizeof(keyboard_buffer)) {
			buffer_write_ptr = 0;
		}
	}
}

void keyboard_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
	uint8_t new_scan_code = inb(0x60);

	if (escaped) {
		new_scan_code += 256;
	}
	switch(new_scan_code) {
		case 0x2a:
		case 0x36:
			shift_state = true;
		break;
		case 0x3A:
			caps_lock = true;
		break;
		case 0xBA:
			caps_lock = false;
		break;
		case 0xaa:
		case 0xb6:
			shift_state = false;
		break;
		case 0x1d:
			ctrl_state = true;
		break;
		case 0x9d:
			ctrl_state = false;
		break;
		case 0x38:
			alt_state = true;
		break;
		case 0xb8:
			alt_state = false;
		break;
		case 0xe0:
			escaped = true;
		break;
		default:
			if ((new_scan_code & 0x80) == 0) {
				// key press (make)
				if (!key_states[new_scan_code].down) {
					key_states[new_scan_code].down = true;
					key_states[new_scan_code].ticks_held = 0;

					// First event: translate and push
					char x = translate_keycode(new_scan_code, escaped, shift_state, ctrl_state, alt_state);
					if (x) push_to_buffer(x);
				}
			} else {
				// key release (break)
				key_states[new_scan_code & 0x7F].down = false;
			}
 		break;
	}

}


unsigned char kgetc([[maybe_unused]] console* cons)
{
	if (buffer_read_ptr >= buffer_write_ptr) {
		return 255;
	}

	return keyboard_buffer[buffer_read_ptr++];
}

