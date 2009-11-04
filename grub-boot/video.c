#include "io.h"
#include "video.h"

/* Internal use: graphics buffer address in flat memory model */
static unsigned char* video = (unsigned char*) VIDEO_MEMORY;

/* Clear the screen */
void clearscreen(console* c)
{
	unsigned char* vptr = video;
	for(; vptr < (unsigned char*) VIDEO_MEMORY_END;)
	{
		/* Because we want to set the default colour on
		 * all cells we can't just roll over the entire
		 * screen with \0.
		 */
		*vptr++ = ' ';
		*vptr++ = c->attributes;
	}
	c->x = 0;
	c->y = 0;
	setcursor(c);
}

/* Moves a line from the source to destination. This is used internally
 * by scroll_screen()
 */
void relocate_line(console* c, unsigned int source, unsigned int dest)
{
	unsigned int sourcepos = source * SCREEN_WIDTH_BYTES;
	unsigned int destpos = dest * SCREEN_WIDTH_BYTES;
	int chars = 0;
	for (; chars < SCREEN_WIDTH_BYTES; chars++)
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
	if (c->y > SCREEN_HEIGHT)
	{
		scroll_screen(c);
		c->y = SCREEN_HEIGHT;
	}
}

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(console* c, const char n)
{
	switch (n)
	{
		case '\0':
			return;
		break;
		case '\n':
			c->x = 0;
			c->y++;
			setcursor(c);
			return;
		break;
	}

	unsigned int pos = (c->x * 2) + (c->y * SCREEN_WIDTH_BYTES);

	video[pos] = n;
	video[pos + 1] = c->attributes;

	c->x++;
	if (c->x > SCREEN_WIDTH - 1)
	{
		c->x = 0;
		c->y++;
	}
	setcursor(c);
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(console* c, char* message)
{
	for(; *message; ++message)
		put(c, *message);
}

void blitconsole(console* c)
{
	/* TODO: Optimised memcpy here */
	unsigned char* vptr = (unsigned char*)video;
	unsigned int n = 0;
	for(; vptr < (unsigned char*) VIDEO_MEMORY_END;)
		*vptr++ = c->video[n++];

	/* For writing to video memory we have to multiply offsets by 2
	 * to account for text attributes for each cell, but when setting
	 * the cursor we do not need to do this.
	 *
	 * Set the cursor by writing to the CRTC I/O ports.
	 */
	unsigned int cursorpos = (c->x) + (c->y * SCREEN_WIDTH);
	outb(0x3D4, 14);                    // write to register 14 first
	outb(0x3D5, (cursorpos >> 8) & 0xFF); // output high byte
	outb(0x3D4, 15);                    // again to register 15
	outb(0x3D5, cursorpos & 0xFF);      // low byte in this register
}

void initconsole(console* c)
{
	c->x = c->y = 0;
	c->attributes = DEFAULT_COLOUR;
	int x = 0;
	for (; x < SCREEN_SIZE; x += 2)
	{
		c->video[x] = ' ';
		c->video[x+1] = c->attributes;
	}
}

