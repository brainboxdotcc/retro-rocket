/**
 * @file keyboard.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

/**
 * @brief Extended keys.
 * BASIC reports these keypresses directly via INKEY$
 */
typedef enum extended_key_t {
	KEY_PAGEUP = 245,
	KEY_PAGEDOWN = 246,
	KEY_DEL = 247,
	KEY_INS = 248,
	KEY_END = 249,
	KEY_UP = 250,
	KEY_DOWN = 251,
	KEY_LEFT = 252,
	KEY_RIGHT = 253,
	KEY_HOME = 254,
} extended_key_t;

/**
 * @brief Claim keyboard interrupt
 */
void init_keyboard();

/**
 * @brief Returns next key in circular buffer, or 255 if no key available
 * 
 * @param cons current console
 * @return unsigned char key from circular buffer
 */
unsigned char kgetc(console* cons);

/**
 * @brief Returns true if the CTRL key is held down
 * 
 * @return true if CTRL key held
 */
bool ctrl_held();

/**
 * @brief Returns true if the SHIFT key is held down
 * 
 * @return true if SHIFT key held
 */
bool shift_held();

/**
 * @brief Returns true if the ALT key is held down
 * 
 * @return true if ALT key held
 */
bool alt_held();

/**
 * @brief Returns true if the CAPS LOCK key is enganged
 * 
 * @return true if CAPS LOCK is on
 */
bool caps_lock_on();