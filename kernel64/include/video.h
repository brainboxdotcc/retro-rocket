#ifndef __VIDEO_H__
#define __VIDEO_H__

/* Colour names */
#define COLOUR_BLACK 0
#define COLOUR_DARKBLUE 1
#define COLOUR_DARKGREEN 2
#define COLOUR_DARKCYAN 3
#define COLOUR_DARKRED 4
#define COLOUR_DARKMAGENTA 5
#define COLOUR_ORANGE 6
#define COLOUR_WHITE 7
#define COLOUR_GREY 8
#define COLOUR_LIGHTBLUE 9
#define COLOUR_LIGHTGREEN 10
#define COLOUR_LIGHTCYAN 11
#define COLOUR_LIGHTRED 12
#define COLOUR_LIGHTMAGENTA 13
#define COLOUR_LIGHTYELLOW 14
#define COLOUR_LIGHTWHITE 15

/* Default background/foreground colour for text output (white on black) */
#define DEFAULT_COLOUR COLOUR_WHITE

/* Tab width for \t character in output */
#define TAB_WIDTH 6

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
#define VIDEO_MEMORY_SIZE 0x1000

/* Structure to represent a console screen.
 * Low level output functions such as put() operate
 * against one of these structures which must be displayed
 * to video ram using blitconsole().
 */
typedef struct
{
	char dirty;
	unsigned int x;
	unsigned int y;
	unsigned char attributes;
	unsigned char* video;
} console;

/* Prototypes for simple console functions */

/* Clear the screen and set cursor to 0,0 */
void clearscreen();

/* Set cursor position to given coordinates */
void setcursor();

/* Output a null terminated C string at the given cursor coordinates then update the
 * cursor coordinates to the end of the string, scrolling the screen if needed.
 */
void putstring(char* message);

/* Output a character to the screen at the given cursor coordinates then update the
 * cursor coordinates to the cell after the character, scrolling the screen if needed.
 * This is called internally by putstring().
 */
void put(const char n);

void initconsole();

void setbackground(unsigned char background);

void setforeground(unsigned char foreground);

#endif
