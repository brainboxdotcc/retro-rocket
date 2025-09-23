/**
 * @file video.h
 * @author Craig Edwards
 * @brief Public API for Retro Rocket video and console output.
 *
 * Provides framebuffer access, console abstraction, ANSI colour control,
 * pixel operations, and Flanterm-backed text rendering. These functions
 * are intended for kernel subsystems and BASIC userland output.
 *
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

/* VGA/ANSI-compatible colour indices */
#define COLOUR_BLACK        0
#define COLOUR_DARKBLUE     1
#define COLOUR_DARKGREEN    2
#define COLOUR_DARKCYAN     3
#define COLOUR_DARKRED      4
#define COLOUR_DARKMAGENTA  5
#define COLOUR_ORANGE       6
#define COLOUR_WHITE        7
#define COLOUR_GREY         8
#define COLOUR_LIGHTBLUE    9
#define COLOUR_LIGHTGREEN   10
#define COLOUR_LIGHTCYAN    11
#define COLOUR_LIGHTRED     12
#define COLOUR_LIGHTMAGENTA 13
#define COLOUR_LIGHTYELLOW  14
#define COLOUR_LIGHTWHITE   15

/* VGA foreground colours as ANSI escapes */
#define VGA_FG_BLACK        "\x1b[30m"
#define VGA_FG_BLUE         "\x1b[34m"
#define VGA_FG_GREEN        "\x1b[32m"
#define VGA_FG_CYAN         "\x1b[36m"
#define VGA_FG_RED          "\x1b[31m"
#define VGA_FG_MAGENTA      "\x1b[35m"
#define VGA_FG_BROWN        "\x1b[33m"
#define VGA_FG_WHITE        "\x1b[37m"
#define VGA_FG_GRAY         "\x1b[90m"
#define VGA_FG_LIGHTBLUE    "\x1b[94m"
#define VGA_FG_LIGHTGREEN   "\x1b[92m"
#define VGA_FG_LIGHTCYAN    "\x1b[96m"
#define VGA_FG_LIGHTRED     "\x1b[91m"
#define VGA_FG_LIGHTMAGENTA "\x1b[95m"
#define VGA_FG_YELLOW       "\x1b[93m"
#define VGA_FG_LIGHTWHITE   "\x1b[97m"

/* VGA background colours as ANSI escapes */
#define VGA_BG_BLACK        "\x1b[40m"
#define VGA_BG_BLUE         "\x1b[44m"
#define VGA_BG_GREEN        "\x1b[42m"
#define VGA_BG_CYAN         "\x1b[46m"
#define VGA_BG_RED          "\x1b[41m"
#define VGA_BG_MAGENTA      "\x1b[45m"
#define VGA_BG_BROWN        "\x1b[43m"
#define VGA_BG_WHITE        "\x1b[47m"
#define VGA_BG_GRAY         "\x1b[100m"
#define VGA_BG_LIGHTBLUE    "\x1b[104m"
#define VGA_BG_LIGHTGREEN   "\x1b[102m"
#define VGA_BG_LIGHTCYAN    "\x1b[106m"
#define VGA_BG_LIGHTRED     "\x1b[101m"
#define VGA_BG_LIGHTMAGENTA "\x1b[105m"
#define VGA_BG_YELLOW       "\x1b[103m"
#define VGA_BG_LIGHTWHITE   "\x1b[107m"

/* Reset to defaults */
#define VGA_RESET           "\x1b[0m"


/** @brief Default text colour (white on black). */
#define DEFAULT_COLOUR COLOUR_WHITE

/* -------------------------------------------------------------------------- */
/* Console text functions                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clear the screen and reset cursor to (0,0).
 */
void clearscreen();

/**
 * @brief Write a null-terminated string to the console.
 *
 * Text begins at the current cursor location. Cursor is updated
 * afterwards, and scrolling occurs if needed.
 *
 * @param c Target console.
 * @param message String to display.
 */
void putstring(const char* message);

/**
 * @brief Write a string to the debug port (0xE9) only.
 * @param message String to output.
 */
void dputstring(const char* message);

/**
 * @brief Write a single character to the console.
 *
 * Cursor is advanced after output, and the screen may scroll.
 *
 * @param n Character to display.
 */
void put(const char n);

/**
 * @brief Write a single character directly to the debug port (0xE9).
 * @param n Character to output.
 */
void dput(const char n);

/**
 * @brief Initialise the primary console and framebuffer.
 *
 * Called during kernel startup. Detects the framebuffer via Limine,
 * initialises Flanterm, loads a font module (if present), and prepares
 * the first console for output.
 */
void init_console();

/**
 * @brief Set the background colour for subsequent text.
 * @param background VGA colour index (0–15).
 */
void setbackground(unsigned char background);

/**
 * @brief Set the foreground (text) colour for subsequent output.
 * @param foreground VGA colour index (0–15).
 */
void setforeground(unsigned char foreground);

/* -------------------------------------------------------------------------- */
/* Screen geometry and framebuffer                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get the framebuffer height in pixels.
 * @return Screen height in pixels.
 */
int16_t screen_get_height();

/**
 * @brief Get the framebuffer width in pixels.
 * @return Screen width in pixels.
 */
int16_t screen_get_width();

/**
 * @brief Draw a pixel to the backbuffer.
 * @param x Pixel X coordinate.
 * @param y Pixel Y coordinate.
 * @param rgb 32-bit RGB value.
 */
void putpixel(int64_t x, int64_t y, uint32_t rgb);

/**
 * @brief Read a pixel from the backbuffer.
 * @param x Pixel X coordinate.
 * @param y Pixel Y coordinate.
 * @return 32-bit RGB value at the given position.
 */
uint32_t getpixel(int64_t x, int64_t y);

/**
 * @brief Get the base address of the backbuffer.
 * @return 64-bit physical address of the framebuffer backbuffer.
 */
uint64_t framebuffer_address();

/**
 * @brief Calculate pixel offset in framebuffer from coordinates.
 * @param x Pixel X coordinate.
 * @param y Pixel Y coordinate.
 * @return Byte offset into framebuffer; 0 if out of bounds.
 */
uint64_t pixel_address(int64_t x, int64_t y);

/* -------------------------------------------------------------------------- */
/* Text cursor and dimensions                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get the console width in text cells (columns).
 * @return Width in characters.
 */
uint64_t get_text_width();

/**
 * @brief Get the console height in text cells (rows).
 * @return Height in characters.
 */
uint64_t get_text_height();

/**
 * @brief Move the cursor to a specific (x,y) cell.
 * @param x Column position.
 * @param y Row position.
 */
void gotoxy(uint64_t x, uint64_t y);

/**
 * @brief Query the current cursor position.
 *
 * Issues an ANSI position request to the terminal backend.
 * Blocks until a response is received.
 *
 * @param x Pointer to store current column.
 * @param y Pointer to store current row.
 */
void get_text_position(uint64_t* x, uint64_t* y);

/* -------------------------------------------------------------------------- */
/* Framebuffer and flipping                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialise framebuffer backbuffer from Limine response.
 *
 * Allocates a backbuffer equal in size to the primary framebuffer.
 * Must be called before drawing or flipping.
 */
void rr_console_init_from_limine();

/**
 * @brief Copy backbuffer to the frontbuffer if dirty.
 *
 * If auto-flip is enabled, this occurs automatically after writes.
 * Otherwise, call rr_flip() manually to refresh the display.
 */
void rr_flip(void);

/**
 * @brief Query whether auto-flip mode is enabled.
 * @return true if auto-flip is active, false otherwise.
 */
bool video_flip_auto(void);

/**
 * @brief Enable or disable auto-flip mode.
 * @param flip true to enable, false to disable.
 */
void set_video_auto_flip(bool flip);

void set_video_dirty_area(int64_t start, int64_t end);

const char* ansi_colour(char *out, size_t out_len, unsigned char vga_colour, bool background);

unsigned char map_vga_to_ansi(unsigned char colour);

unsigned char map_vga_to_ansi_bg(unsigned char colour);

void redefine_character(unsigned char c, uint8_t bitmap[8]);

void graphics_putstring(const char *s, int64_t x, int64_t y, int32_t colour);

int add_scrollable(int64_t start, int64_t end);