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

void draw_horizontal_line(int64_t from_x, int64_t to_x, int64_t y, uint32_t colour);

void draw_horizontal_rectangle(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour);

void draw_triangle(int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3, uint32_t colour);