#include <kernel.h>

static int64_t screen_x = 0, screen_y = 0, current_x = 0, current_y = 0;
static console first_console;
bool wait_state = false;
console* current_console = NULL;

void terminal_callback(struct limine_terminal*, uint64_t, uint64_t, uint64_t, uint64_t);

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0,
    .callback = terminal_callback,
};

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
};

inline uint64_t framebuffer_address()
{
	struct limine_framebuffer *fb = terminal_request.response->terminals[0]->framebuffer;
	return (uint64_t)fb->address;
}

inline uint64_t pixel_address(int64_t x, int64_t y)
{
	uint64_t pitch = terminal_request.response->terminals[0]->framebuffer->pitch;
	uint64_t bytes_per_pixel = terminal_request.response->terminals[0]->framebuffer->bpp >> 3;
	if (x >= 0 && y >= 0 && x < screen_x && y < screen_y) {
		return (y * pitch) + (x * bytes_per_pixel);
	}
	return 0;
}

uint64_t get_text_width()
{
	return terminal_request.response->terminals[0]->columns;
}

uint64_t get_text_height()
{
	return terminal_request.response->terminals[0]->rows;
}

void screenonly(console* c, const char* s)
{
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, s, strlen(s));
}

void get_text_position(uint64_t* x, uint64_t* y)
{
	wait_state = true;
	putstring(current_console, "\033[6n");
	while (wait_state);
	*x = current_x;
	*y = current_y;
}

void gotoxy(uint64_t x, uint64_t y)
{
	char cursor_command[32];
	snprintf(cursor_command, 32, "\033[%d;%dH", y % (get_text_height() + 1), x % (get_text_width() + 1));
	screenonly(current_console, cursor_command);
}

void putpixel(int64_t x, int64_t y, uint32_t rgb)
{
	volatile uint32_t* addr = (volatile uint32_t*)(framebuffer_address() + pixel_address(x, y));
	*addr = rgb;
}

uint32_t getpixel(int64_t x, int64_t y)
{
	return *((volatile uint32_t*)(framebuffer_address() + pixel_address(x, y)));
}

/* Clear the screen - note this does not send the ansi to the debug console */
void clearscreen(console* c)
{
	draw_horizontal_rectangle(0, 0, screen_get_width() - 1, screen_get_height() - 1, 0x000000);
	screenonly(c, "\033[2J\033[0;0H");
}

void dput(const char n)
{
	outb(0xE9, n);
}

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(console* c, const char n)
{
	dput(n);
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, &n, 1);
}

void dputstring(char* message)
{
	for (; *message; ++message) {
		outb(0xE9, *message);
		if (*message == 13) {
			outb(0xE9, 10);
		} else if (*message == 10) {
			outb(0xE9, 13);
		}
	}
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(console* c, char* message)
{
	dputstring(message);
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, message, strlen(message));
}

void terminal_callback(struct limine_terminal *terminal, uint64_t type, uint64_t x, uint64_t y, uint64_t z)
{
	switch (type) {
		case LIMINE_TERMINAL_CB_BELL:
			beep(1000);
		break;
		case LIMINE_TERMINAL_CB_POS_REPORT:
			current_x = x - 1;
			current_y = y - 1;
		break;
	}
	wait_state = false;
}

void init_console()
{
	console* c = &first_console;
	current_console = c;
	c->internalbuffer = NULL;
	if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
		dprintf("No limine terminal offered\n");
		wait_forever();
	}
	clearscreen(c);
	dprintf("limine terminal write address=%016X\n", terminal_request.response->write);
	screen_x = terminal_request.response->terminals[0]->framebuffer->width;
	screen_y = terminal_request.response->terminals[0]->framebuffer->height;
	dprintf("Framebuffer address: %llx x resolution=%d y resolution=%d\n", framebuffer_address(), screen_get_width(), screen_get_height());

	setforeground(current_console, COLOUR_LIGHTYELLOW);
	printf("Retro-Rocket ");
	setforeground(current_console, COLOUR_WHITE);
	printf("64-bit SMP kernel booting\n");
}

int16_t screen_get_width()
{
	return screen_x;
}

int16_t screen_get_height()
{
	return screen_y;
}

/*
VGA

$00 Black
$01 Blue
$02 Green
$03 Cyan
$04 Red
$05 Magenta
$06 Brown
$07 White
$08 Gray
$09 Light Blue
$0A Light Green
$0B Light Cyan
$0C Light Red
$0D Light Magenta
$0E Yellow
$0F Bright White

ANSI

Black 	30
Red 	31
Green 	32
Yellow 	33
Blue 	34
Magenta 35
Cyan 	36
White 	37
*/
unsigned char map_vga_to_ansi(unsigned char colour)
{
	switch (colour) {
		case 0x00: // Black
			return 30;
		case 0x01: // Blue
			return 34;
		case 0x02: // Green
			return 32;
		case 0x03: // Cyan
			return 36;
		case 0x04: // Red
			return 31;
		case 0x05: // Magenta
			return 35;
		case 0x06: // Brown
			return 33;
		case 0x07: // White
			return 37;
		case 0x08: // Gray
			return 90;
		case 0x09: // Light Blue
			return 94;
		case 0x0A: // Light Green
			return 92;
		case 0x0B: // Light Cyan
			return 96;
		case 0x0C: // Light Red
			return 91;
		case 0x0D: // Light Magenta
			return 95;
		case 0x0E: // Yellow
			return 93;
		default: // Bright White
			return 97;
	}
}

void setbackground(console* c, unsigned char background)
{
	char code[100];
	snprintf(code, 100, "%c[%dm", 27, background + 40);
	putstring(c, code);
}

void setforeground(console* c, unsigned char foreground)
{
	char code[100];
	snprintf(code, 100, "%c[%dm", 27, map_vga_to_ansi(foreground));
	putstring(c, code);
}

