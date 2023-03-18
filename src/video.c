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

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(console* c, const char n)
{
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, &n, 1);
	outb(0xE9, n);
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(console* c, char* message)
{
	struct limine_terminal *terminal = terminal_request.response->terminals[0];
	terminal_request.response->write(terminal, message, strlen(message));
	for (; *message; ++message) {
		outb(0xE9, *message);
		if (*message == 13) {
			outb(0xE9, 10);
		} else if (*message == 10) {
			outb(0xE9, 13);
		}
	}
}

void initconsole(console* c)
{
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

