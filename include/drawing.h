/**
 * @file drawing.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#pragma once

#include "kernel.h"

typedef enum coordinate_range_type_t {
	RANGE_X,
	RANGE_Y,
} coordinate_range_type_t;

/**
 * @brief Draw a straight line from from_x,from_y to to_x,to_y in the given colour
 * 
 * @param from_x starting X coordinate
 * @param from_y starting Y coordinate
 * @param to_x ending X coordinate
 * @param to_y ending Y coordinate
 * @param colour RGB colour to use 
 */
void draw_line(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour);

/**
 * @brief Draw a horizontal line.
 * @note If you know the line you want to draw is going to be horizontal
 * this is faster than the bresenham based line drawing offered by
 * draw_line() as its is a simple for() loop.
 * 
 * @param from_x Starting X coordinate
 * @param to_x Ending X coordinate
 * @param y Y coordinate
 * @param colour Colour to fill the line
 */
void draw_horizontal_line(int64_t from_x, int64_t to_x, int64_t y, uint32_t colour);

/**
 * @brief Draw a horizontal rectangle using draw_horizontal_line()
 * 
 * @param from_x X coordinate of first corner
 * @param from_y Y coordinate of first corner
 * @param to_x X coordinate of opposite corner
 * @param to_y Y coordinate of opposite corner
 * @param colour colour to fill the rectangle
 */
void draw_horizontal_rectangle(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour);

/**
 * @brief Draw an arbitrary filled triangle
 * 
 * @param x1 X coordinate of first corner
 * @param y1 Y coordinate of first corner
 * @param x2 X coordinate of second corner
 * @param y2 Y coordinate of second corner
 * @param x3 X coordinate of third corner
 * @param y3 Y coordinate of third corner
 * @param colour Colour to fill triangle
 */
void draw_triangle(int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3, uint32_t colour);

/**
 * @brief Draw a filled or solid circle.
 * @note Filled circles are rendered using horizontal lines via the
 * draw_horizontal_line() function
 * 
 * @param x_centre Centre X coordinate
 * @param y_centre Centre Y coordinate
 * @param radius Radius of circle
 * @param fill True to fill the circle, false to just draw the outline
 * @param colour Colour of circle to draw
 */
void draw_circle(int64_t x_centre, int64_t y_centre, int64_t radius, bool fill, uint32_t colour);