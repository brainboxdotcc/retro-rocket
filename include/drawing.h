#pragma once

#include "kernel.h"

typedef enum coordinate_range_type_t {
	RANGE_X,
	RANGE_Y,
} coordinate_range_type_t;

void draw_line(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour);