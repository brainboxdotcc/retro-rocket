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

/* Structure to represent a console screen.
 * Low level output functions such as put() operate
 * against one of these structures.
 */
typedef struct
{
	char dirty;
	unsigned int x;
	unsigned int y;
	unsigned char attributes;
	unsigned char last;
	char* internalbuffer;
	char* buffer;
	int bufcnt;
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

void initconsole(console* c);

void setbackground(console* c, unsigned char background);

void setforeground(console* c, unsigned char foreground);

extern console* current_console;

#endif
