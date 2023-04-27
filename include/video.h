/**
 * @file video.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once

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
	uint8_t x;
	uint8_t y;
	uint8_t attributes;
	uint8_t last;
	char* internalbuffer;
	char* buffer;
	size_t bufcnt;
} console;

/* Prototypes for simple console functions */

/* Clear the screen and set cursor to 0,0 */
void clearscreen(console* c);

/* Set cursor position to given coordinates */
void setcursor(console* c);

/* Output a null terminated C string at the given cursor coordinates then update the
 * cursor coordinates to the end of the string, scrolling the screen if needed.
 */
void putstring(console* c, const char* message);
void dputstring(const char* message);

/* Output a character to the screen at the given cursor coordinates then update the
 * cursor coordinates to the cell after the character, scrolling the screen if needed.
 * This is called internally by putstring().
 */
void put(console* c, const char n);
void dput(const char n);

void init_console();

void setbackground(console* c, unsigned char background);

void setforeground(console* c, unsigned char foreground);

int16_t screen_get_height();

int16_t screen_get_width();

void putpixel(int64_t x, int64_t y, uint32_t rgb);

uint32_t getpixel(int64_t x, int64_t y);

uint64_t framebuffer_address();

uint64_t pixel_address(int64_t x, int64_t y);

uint64_t get_text_width();

uint64_t get_text_height();

void gotoxy(uint64_t x, uint64_t y);

void get_text_position(uint64_t* x, uint64_t* y);

extern console* current_console;

