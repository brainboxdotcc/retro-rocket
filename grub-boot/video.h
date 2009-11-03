#ifndef __VIDEO_H__
#define __VIDEO_H__

/* Simple structure to represent a screen cursor position */
typedef struct
{
	unsigned int x;
	unsigned int y;
} cursor;

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

/* Prototypes for simple console functions */

/* Clear the screen and set cursor to 0,0 */
void clearscreen(cursor* c);

/* Set cursor position to given coordinates */
void setcursor(cursor* c);

/* Output a null terminated C string at the given cursor coordinates then update the
 * cursor coordinates to the end of the string, scrolling the screen if needed.
 */
void putstring(cursor* c, char* message);

/* Output a character to the screen at the given cursor coordinates then update the
 * cursor coordinates to the cell after the character, scrolling the screen if needed.
 * This is called internally by putstring().
 */
void put(cursor* c, const char n);

#endif
