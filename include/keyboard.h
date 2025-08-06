/**
 * @file keyboard.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 *
 * @brief Low-level keyboard input driver for Retro Rocket.
 *
 * This module handles:
 *   - PS/2 scan code to ASCII translation (UK layout by default).
 *   - Modifier key state tracking (Shift, Ctrl, Alt, Caps Lock).
 *   - Buffered keypress input for use by the console/OS.
 *   - Extended keycodes for non-ASCII keys (arrows, paging, etc).
 *   - Key repeat timing control.
 *
 * BASIC programs see input via INKEY$, which may return ASCII
 * characters or extended key constants defined here.
 */

#pragma once

/**
 * @brief Delay before held keys begin to auto-repeat.
 *
 * Measured in scheduler ticks (centi-seconds, i.e. 1/100 s).
 * A value of 25 = 250 ms delay before repeat starts.
 */
#define REPEAT_DELAY_TICKS 25

/**
 * @brief Interval between auto-repeats once a key is held down.
 *
 * Measured in scheduler ticks (centi-seconds, i.e. 1/100 s).
 * A value of 5 = 50 ms between repeats (~20 chars/sec).
 */
#define REPEAT_RATE_TICKS   5

/**
 * @brief Size of the circular keyboard buffer in bytes.
 *
 * All translated keypresses are queued here. When the buffer
 * fills, new input will overwrite oldest unread characters.
 */
#define KEYBOARD_BUFFER_SIZE 1024

#define MAX_SCANCODE 128

/**
 * @brief Extended (non-ASCII) keys.
 *
 * These are returned by @ref kgetc() when pressed.
 * BASIC exposes them directly via INKEY$.
 */
typedef enum extended_key_t {
	KEY_PAGEUP   = 245,
	KEY_PAGEDOWN = 246,
	KEY_DEL      = 247,
	KEY_INS      = 248,
	KEY_END      = 249,
	KEY_UP       = 250,
	KEY_DOWN     = 251,
	KEY_LEFT     = 252,
	KEY_RIGHT    = 253,
	KEY_HOME     = 254,
} extended_key_t;

/**
 * @brief Internal structure for key state tracking.
 *
 * Used by the auto-repeat logic to record whether a key
 * is currently pressed and how long it has been held.
 */
struct key_state {
	bool down;           ///< True if key is currently pressed
	uint16_t ticks_held; ///< Number of ticks the key has been held
};

/**
 * @brief Initialises the keyboard driver.
 *
 * Claims IRQ1, sets up the scan code map, and clears the
 * keyboard buffer ready for use.
 */
void init_keyboard();

/**
 * @brief Reads the next key from the keyboard buffer.
 *
 * @param cons Current console (unused, reserved for future use).
 * @return ASCII character or extended key constant,
 *         or 255 if no input is available.
 */
unsigned char kgetc(console* cons);

/**
 * @brief Tests if the CTRL key is currently held down.
 *
 * @return true if CTRL is held, false otherwise.
 */
bool ctrl_held();

/**
 * @brief Tests if the SHIFT key is currently held down.
 *
 * @return true if SHIFT is held, false otherwise.
 */
bool shift_held();

/**
 * @brief Tests if the ALT key is currently held down.
 *
 * @return true if ALT is held, false otherwise.
 */
bool alt_held();

/**
 * @brief Tests if CAPS LOCK is currently engaged.
 *
 * @return true if CAPS LOCK is on, false otherwise.
 */
bool caps_lock_on();

void load_keymap_from_string(const char* text);

bool key_waiting();