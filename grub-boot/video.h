#ifndef __VIDEO_H__
#define __VIDEO_H__

/* Default background/foreground colour for text output (white on black) */
#define DEFAULT_COLOUR 0x07

/* Screen width and height of default display mode */
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24

/* Screen widths and heights in bytes */
#define SCREEN_WIDTH_BYTES SCREEN_WIDTH * 2
#define SCREEN_SIZE SCREEN_WIDTH * 2 * SCREEN_HEIGHT
#define SCREEN_LAST_CELL (SCREEN_HEIGHT + 1) * SCREEN_WIDTH_BYTES

/* Address in flat memory of video ram */
#define VIDEO_MEMORY 0xB8000
#define VIDEO_MEMORY_END 0xB9000

/* Structure to represent a console screen.
 * In the future this will also include the actual
 * screen buffer and we will use a function to blit
 * it to the screen memory depending on which console
 * is being displayed. When blitting, this will update
 * the cursor location
 */
typedef struct
{
	unsigned int x;
	unsigned int y;
	unsigned char attributes;
	unsigned char video[SCREEN_LAST_CELL];
} console;

/* Prototypes for simple console functions */

/* Clear the screen and set cursor to 0,0 */
void clearscreen(console* c);

/* Set cursor position to given coordinates */
void setcursor(console* c);

/* Output a null terminated C string at the given cursor coordinates then update the
 * cursor coordinates to the end of the string, scrolling the screen if needed.
 */
void putstring(console* c, char* message);

/* Output a character to the screen at the given cursor coordinates then update the
 * cursor coordinates to the cell after the character, scrolling the screen if needed.
 * This is called internally by putstring().
 */
void put(console* c, const char n);

void blitconsole(console* c);

void initconsole(console* c);

void setbackground(console* c, unsigned char background);

void setforeground(console* c, unsigned char foreground);

#endif
