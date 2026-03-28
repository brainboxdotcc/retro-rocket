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
 * @brief Advance to the next frame in an animated gif sprite,
 * or reset to the first frame
 * (ANIMATE NEXT s, ANIMATE RESET s)
 *
 * @param ctx The current BASIC context.
 */
void animate_statement(struct basic_ctx* ctx);

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

/**
 * @brief Implements the BASIC `PLOTQUAD` statement.
 *
 * Parses and executes the `PLOTQUAD` keyword from the BASIC interpreter.
 * This statement expects a sprite handle followed by four (x,y) coordinate
 * pairs, defining the quadrilateral to which the sprite should be mapped.
 *
 * The coordinates may describe any convex quad, allowing sprites to be drawn
 * with skew and perspective. Internally this delegates to the textured-quad
 * blitter (`plot_sprite_quad`) to perform the actual rendering.
 *
 * Example usage in BASIC:
 * @code
 *   PLOTQUAD SPRITEHANDLE, 100,100, 200,120, 190,220, 90,210
 * @endcode
 *
 * @param ctx Pointer to the BASIC execution context, containing parser state,
 *            sprite table, and current program environment.
 */
void plotquad_statement(struct basic_ctx* ctx);

/**
 * @brief Check for pixel-perfect collision between two sprites
 *
 * Performs an axis-aligned bounding box (AABB) test followed by a
 * per-pixel mask intersection test to determine whether two sprites
 * overlap on any opaque pixels.
 *
 * BASIC usage:
 *   result = SPRITECOLLIDE(sprite_a, ax, ay, sprite_b, bx, by)
 *
 * @param ctx BASIC execution context
 * @return 1 if the sprites collide, 0 otherwise
 */
int64_t basic_spritecollide(struct basic_ctx* ctx);

/**
 * @brief BASIC binding for SPRITEWIDTH.
 *
 * Returns the width of the specified sprite in pixels.
 *
 * @param ctx BASIC execution context.
 * @return Sprite width in pixels, or 0 on error.
 */
int64_t basic_spritewidth(struct basic_ctx* ctx);

/**
 * @brief BASIC binding for SPRITEHEIGHT.
 *
 * Returns the height of the specified sprite in pixels.
 *
 * @param ctx BASIC execution context.
 * @return Sprite height in pixels, or 0 on error.
 */
int64_t basic_spriteheight(struct basic_ctx* ctx);