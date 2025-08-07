#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Get the RGB color value from the provided red, green, and blue components.
 *
 * @param ctx The current BASIC context.
 * @return The combined RGB value as a 32-bit integer.
 */
int64_t basic_rgb(struct basic_ctx* ctx);

/**
 * @brief Draw a circle on the screen at the specified coordinates with a specified radius.
 *
 * @param ctx The current BASIC context.
 */
void circle_statement(struct basic_ctx* ctx);

/**
 * @brief Draw a triangle on the screen defined by the three sets of x, y coordinates.
 *
 * @param ctx The current BASIC context.
 */
void triangle_statement(struct basic_ctx* ctx);

/**
 * @brief Plot a single point on the screen at the specified x and y coordinates.
 *
 * @param ctx The current BASIC context.
 */
void point_statement(struct basic_ctx* ctx);

/**
 * @brief Draw a line from one point to another on the screen.
 *
 * @param ctx The current BASIC context.
 */
void draw_line_statement(struct basic_ctx* ctx);

/**
 * @brief Set the graphics color for subsequent drawing operations.
 *
 * @param ctx The current BASIC context.
 */
void gcol_statement(struct basic_ctx* ctx);

/**
 * @brief Draw a rectangle on the screen defined by two sets of x, y coordinates.
 *
 * @param ctx The current BASIC context.
 */
void rectangle_statement(struct basic_ctx* ctx);

/**
 * @brief Load a sprite from a file and assign it to a variable.
 *
 * @param ctx The current BASIC context.
 */
void loadsprite_statement(struct basic_ctx* ctx);

/**
 * @brief Free a loaded sprite from memory.
 *
 * @param ctx The current BASIC context.
 * @param sprite_handle The handle to the sprite to be freed.
 */
void freesprite_statement(struct basic_ctx* ctx);

/**
 * @brief Plot a previously loaded sprite at the specified coordinates.
 *
 * @param ctx The current BASIC context.
 */
void plot_statement(struct basic_ctx* ctx);

/**
 * @brief Free the memory allocated for a sprite.
 *
 * @param ctx The current BASIC context.
 * @param sprite_handle The handle to the sprite to be freed.
 */
void free_sprite(struct basic_ctx* ctx, int64_t sprite_handle);

/**
 * @brief Enable or disable the automatic video flipping for graphics.
 *
 * @param ctx The current BASIC context.
 */
void autoflip_statement(struct basic_ctx* ctx);

/**
 * @brief Manually flip the video buffer to display the current drawing.
 *
 * @param ctx The current BASIC context.
 */
void flip_statement(struct basic_ctx* ctx);
