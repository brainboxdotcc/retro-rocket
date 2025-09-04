/**
 * @file basic/console.h
 * @brief Header for BASIC console IO functions
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Get the maximum X position for text output.
 *
 * @param ctx The BASIC context.
 * @return The maximum X position for text output.
 */
int64_t basic_get_text_max_x(struct basic_ctx* ctx);

/**
 * @brief Get the maximum Y position for text output.
 *
 * @param ctx The BASIC context.
 * @return The maximum Y position for text output.
 */
int64_t basic_get_text_max_y(struct basic_ctx* ctx);

/**
 * @brief Retrieves the maximum number of characters that can be displayed in
 * a line on the console (width).
 *
 * This function queries the console for the maximum horizontal space available
 * to display text. It returns the number of characters that can fit in a line,
 * allowing for accurate text positioning and wrapping.
 *
 * @param ctx The interpreter context.
 * @return The maximum number of characters that can be displayed in a line.
 */
int64_t basic_get_text_max_x(struct basic_ctx* ctx);


/**
 * @brief Retrieves the maximum number of lines that can be displayed on the console (height).
 *
 * This function queries the console for the maximum vertical space available to
 * display text. It returns the maximum number of lines that can fit on the screen
 * before the text wraps or scrolls.
 *
 * @param ctx The interpreter context.
 * @return The maximum number of lines that can be displayed on the console.
 */
int64_t basic_get_text_max_y(struct basic_ctx* ctx);


/**
 * @brief Retrieves the current horizontal position (column) of the cursor in the console.
 *
 * This function queries the console for the current cursor position along the X-axis
 * (horizontal position). It returns the current column where the cursor is located,
 * which can be used to position or move the cursor programmatically.
 *
 * @param ctx The interpreter context.
 * @return The current column position of the cursor.
 */
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);


/**
 * @brief Retrieves the current vertical position (row) of the cursor in the console.
 *
 * This function queries the console for the current cursor position along the Y-axis
 * (vertical position). It returns the current row where the cursor is located, allowing
 * for precise text positioning and manipulation.
 *
 * @param ctx The interpreter context.
 * @return The current row position of the cursor.
 */
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);

/**
 * @brief Get the current X position of the cursor.
 *
 * @param ctx The BASIC context.
 * @return The current X position of the cursor.
 */
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);

/**
 * @brief Get the current Y position of the cursor.
 *
 * @param ctx The BASIC context.
 * @return The current Y position of the cursor.
 */
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);

/**
 * @brief Check if the CTRL key is held down.
 *
 * @param ctx The BASIC context.
 * @return 1 if CTRL is held, 0 otherwise.
 */
int64_t basic_ctrlkey(struct basic_ctx* ctx);

/**
 * @brief Check if the SHIFT key is held down.
 *
 * @param ctx The BASIC context.
 * @return 1 if SHIFT is held, 0 otherwise.
 */
int64_t basic_shiftkey(struct basic_ctx* ctx);

/**
 * @brief Check if the ALT key is held down.
 *
 * @param ctx The BASIC context.
 * @return 1 if ALT is held, 0 otherwise.
 */
int64_t basic_altkey(struct basic_ctx* ctx);

/**
 * @brief Check if the CAPS LOCK is on.
 *
 * @param ctx The BASIC context.
 * @return 1 if CAPS LOCK is on, 0 otherwise.
 */
int64_t basic_capslock(struct basic_ctx* ctx);

/**
 * @brief Handle cooperative input for the INPUT statement.
 *
 * @param ctx The BASIC context.
 */
void input_statement(struct basic_ctx* ctx);

/**
 * @brief Process GET statement to read a single keypress.
 *
 * @param ctx The BASIC context.
 */
void kget_statement(struct basic_ctx* ctx);

/**
 * @brief Clear the screen.
 *
 * @param ctx The BASIC context.
 */
void cls_statement(struct basic_ctx* ctx);

/**
 * @brief Move the cursor to the specified position.
 *
 * @param ctx The BASIC context.
 */
void gotoxy_statement(struct basic_ctx* ctx);

/**
 * @brief Output data to the console with the PRINT statement.
 *
 * @param ctx The BASIC context.
 */
void print_statement(struct basic_ctx* ctx);

/**
 * @brief Set the text foreground colour.
 *
 * @param ctx The BASIC context.
 * @param tok The token for the colour.
 */
void colour_statement(struct basic_ctx* ctx, enum token_t tok);

/**
 * @brief Set the text background colour.
 *
 * @param ctx The BASIC context.
 */
void background_statement(struct basic_ctx* ctx);

/**
 * @brief Load a keymap from a file.
 *
 * @param ctx The BASIC context.
 */
void keymap_statement(struct basic_ctx* ctx);

/**
 * @brief Retrieves a keypress from the user.
 *
 * This function waits for a keypress from the user and returns the corresponding
 * character as a string. If no key is pressed, the function enters a wait state
 * until a key is pressed. If the key is `255`, which may indicate a timeout or
 * no input, the function enters a "pause" state and returns an empty string.
 *
 * @param ctx The interpreter context.
 * @return A string containing the key pressed by the user. Returns an empty
 * string if no valid key is pressed (e.g., key value `255`).
 */
char* basic_inkey(struct basic_ctx* ctx);

/**
 * @brief Checks if the control (Ctrl) key is currently pressed.
 *
 * This function queries the state of the control (Ctrl) key on the keyboard.
 * It returns a non-zero value (true) if the Ctrl key is pressed, and 0 (false)
 * if it is not.
 *
 * @param ctx The interpreter context.
 * @return A non-zero value if the Ctrl key is pressed, 0 otherwise.
 */
int64_t basic_ctrlkey(struct basic_ctx* ctx);


/**
 * @brief Checks if the shift key is currently pressed.
 *
 * This function queries the state of the shift key on the keyboard. It returns
 * a non-zero value (true) if the shift key is pressed, and 0 (false) if it is not.
 *
 * @param ctx The interpreter context.
 * @return A non-zero value if the shift key is pressed, 0 otherwise.
 */
int64_t basic_shiftkey(struct basic_ctx* ctx);


/**
 * @brief Checks if the alt key is currently pressed.
 *
 * This function queries the state of the alt key on the keyboard. It returns
 * a non-zero value (true) if the Alt key is pressed, and 0 (false) if it is not.
 *
 * @param ctx The interpreter context.
 * @return A non-zero value if the Alt key is pressed, 0 otherwise.
 */
int64_t basic_altkey(struct basic_ctx* ctx);


/**
 * @brief Checks if the Caps Lock key is currently enabled.
 *
 * This function queries the state of the Caps Lock key. It returns a non-zero value
 * (true) if Caps Lock is enabled (active), and 0 (false) if it is not.
 *
 * @param ctx The interpreter context.
 * @return A non-zero value if Caps Lock is enabled, 0 otherwise.
 */
int64_t basic_capslock(struct basic_ctx* ctx);

/**
 * @brief Waits for a given number of seconds
 *
 * @param ctx The interpreter context.
 */
void sleep_statement(struct basic_ctx* ctx);

bool basic_esc();