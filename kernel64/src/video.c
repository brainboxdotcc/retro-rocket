#include <kernel.h>

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

/* Clear the screen */
void clearscreen(console* c)
{
	putstring(c, "\033[2J");
}

/* Moves a line from the source to destination. This is used internally
 * by scroll_screen()
 */
void relocate_line(console* c, unsigned int source, unsigned int dest)
{
	unsigned int sourcepos = source * SCREEN_WIDTH_BYTES;
	unsigned int destpos = dest * SCREEN_WIDTH_BYTES;
	int chars = 0;
	for (; chars < SCREEN_WIDTH_BYTES * 2; chars++)
	{
		c->video[destpos++] = c->video[sourcepos++];
		++chars;
	}
}

/* Scroll the entire screen up by one line
 */
void scroll_screen(console* c)
{
	unsigned int line = 1;
	for (; line <= SCREEN_HEIGHT; ++line)
		relocate_line(c, line, line - 1);
	unsigned int lastline = SCREEN_HEIGHT * SCREEN_WIDTH_BYTES;
	for (; lastline < SCREEN_LAST_CELL;)
	{
		c->video[lastline++] = ' ';
		c->video[lastline++] = DEFAULT_COLOUR;
	}
}

/* Sets a new cursor position. If the cursor position is greater than
 * the height of the screen it will trigger a scroll by one line
 * and the Y position will be set to the last row which will be cleared.
 */
void setcursor(console* c)
{
	if (c->x > SCREEN_WIDTH - 1)
	{
		c->x = 0;
		c->y++;
	}
	if (c->y > SCREEN_HEIGHT)
	{
		scroll_screen(c);
		c->y = SCREEN_HEIGHT;
	}
	if (c->x < 0)
	{
		c->x = 80 - abs(c->x);
		c->y--;
	}
	if (c->y < 0)
	{
		/* Todo: Handle scrolling downwards */
		c->y = 0;
	}
}

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(console* c, const char n)
{
	if (!c) {
		return;
	}
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, &n, 1);
    	c->dirty = 1;
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(console* c, char* message)
{
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, message, strlen(message));
}

void blitconsole(console* c)
{
}

void initconsole(console* c)
{
	c->attributes = DEFAULT_COLOUR;
	c->internalbuffer = NULL;
	if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
		wait_forever();
	}
	clearscreen(c);
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
	sprintf(code, "%c[%dm", 27, background + 40);
	putstring(c, code);
}

void setforeground(console* c, unsigned char foreground)
{
	char code[100];
	sprintf(code, "%c[%dm", 27, map_vga_to_ansi(foreground));
	putstring(c, code);
}

