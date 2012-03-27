#include <kernel.h>
#include <io.h>

console c;

/* Clear the screen */
void clearscreen()
{
	unsigned int x = 0;
	for (; x < SCREEN_LAST_CELL;)
	{
		c.video[x++] = ' ';
		c.video[x++] = c.attributes;
	}

	c.x = 0;
	c.y = 0;
	setcursor();
}

/* Moves a line from the source to destination. This is used internally
 * by scroll_screen()
 */
void relocate_line(unsigned int source, unsigned int dest)
{
	unsigned int sourcepos = source * SCREEN_WIDTH_BYTES;
	unsigned int destpos = dest * SCREEN_WIDTH_BYTES;
	int chars = 0;
	for (; chars < SCREEN_WIDTH_BYTES * 2; chars++)
	{
		c.video[destpos++] = c.video[sourcepos++];
		++chars;
	}
}

/* Scroll the entire screen up by one line
 */
void scroll_screen()
{
	unsigned int line = 1;
	for (; line <= SCREEN_HEIGHT; ++line)
		relocate_line(line, line - 1);
	unsigned int lastline = SCREEN_HEIGHT * SCREEN_WIDTH_BYTES;
	for (; lastline < SCREEN_LAST_CELL;)
	{
		c.video[lastline++] = ' ';
		c.video[lastline++] = DEFAULT_COLOUR;
	}
}

/* Sets a new cursor position. If the cursor position is greater than
 * the height of the screen it will trigger a scroll by one line
 * and the Y position will be set to the last row which will be cleared.
 */
void setcursor()
{
	if (c.x > SCREEN_WIDTH - 1)
	{
		c.x = 0;
		c.y++;
	}
	if (c.y > SCREEN_HEIGHT)
	{
		scroll_screen();
		c.y = SCREEN_HEIGHT;
	}
        unsigned int cursorpos = (c.x) + (c.y * SCREEN_WIDTH);
	outb(0x3D4, 14);                    // write to register 14 first
	outb(0x3D5, (cursorpos >> 8) & 0xFF); // output high byte
	outb(0x3D4, 15);                    // again to register 15
	outb(0x3D5, cursorpos & 0xFF);      // low byte in this registe
}

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(const char n)
{
	switch (n)
	{
		case '\0':
			return;
		break;
		case '\n':
			c.x = 0;
			c.y++;
			setcursor();
			return;
		break;
		case '\t':
			if (c.x % TAB_WIDTH != 0 || c.x == 0)
				c.x += TAB_WIDTH - (c.x % TAB_WIDTH);
			setcursor();
			return;
		break;
	}

	unsigned int pos = (c.x * 2) + (c.y * SCREEN_WIDTH_BYTES);

	c.video[pos] = n;
	c.video[pos + 1] = c.attributes;

	c.x++;
	setcursor();
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(const char* message)
{
	for (; *message; ++message)
		put(*message);
}

void initconsole()
{
	c.attributes = DEFAULT_COLOUR;
	c.video = (u8*)VIDEO_MEMORY;
	clearscreen();
}

void setbackground(unsigned char background)
{
	c.attributes = (background << 4) | (c.attributes & 0x0F);
}

void setforeground(unsigned char foreground)
{
	c.attributes = (foreground) | (c.attributes & 0xF0);
}

