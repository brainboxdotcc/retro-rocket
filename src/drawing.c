#include <kernel.h>

void swap(int64_t* first, int64_t* second)
{
	*first += *second;
	*second = *first - *second;
	*first = *first - *second;
}

void clamp_range(int64_t* coordinate, coordinate_range_type_t type)
{
	if (type == RANGE_X && *coordinate >= screen_get_width()) {
		*coordinate = screen_get_width() - 1;
	} else if (type == RANGE_Y && *coordinate >= screen_get_height()) {
		*coordinate = screen_get_height() - 1;
	} else if (*coordinate < 0) {
		*coordinate = 0;
	}
}

void draw_line(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour)
{
	if (from_y == to_y) {
		draw_horizontal_line(from_x, to_x, from_y, colour);
		return;
	}

	int64_t dx = labs(from_x - to_x);
	int64_t sx = from_x < to_x ? 1 : -1;
	int64_t dy = -(labs(from_y - to_y));
	int64_t sy = from_y < to_y ? 1 : -1;
	int64_t e2, error = dx + dy;

	while (true) {
		putpixel(from_x, from_y, colour);
		if (from_x == to_x && from_y == to_y) {
			break;
		}
		e2 = 2 * error;
		if (e2 >= dy) {
			if (from_x == to_x) {
				break;
			}
			error += dy;
			from_x += sx;
		}
		if (e2 <= dx) {
			if (from_y == to_y) {
				break;
			}
			error += dx;
			from_y += sy;
		}
	}
}

void draw_horizontal_line(int64_t from_x, int64_t to_x, int64_t y, uint32_t colour)
{
	volatile uint32_t* addr = (volatile uint32_t*)(framebuffer_address() + pixel_address(from_x, y));
	for (; from_x <= to_x; from_x++) {
		*addr = colour;
		addr++;
	}
}

void draw_horizontal_rectangle(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, uint32_t colour)
{
	if (from_x > to_x) {
		swap(&from_x, &to_x);
	}
	if (from_y > to_y) {
		swap(&from_y, &to_y);
	}
	for (; from_y != to_y; ++from_y) {
		draw_horizontal_line(from_x, to_x, from_y, colour);
	}
}

bool in_viewport(int64_t x, int64_t y)
{
	return (x >= 0 && y >= 0 && x < screen_get_width() && y < screen_get_height());
}

int64_t min3(int64_t x, int64_t y, int64_t z)
{
	return (x < ((y < z) ? y : z) ) ? x : ((y < z) ? y : z);
}

int64_t max3(int64_t x, int64_t y, int64_t z)
{
	return (x > ((y > z) ? y : z) ) ? x : ((y > z) ? y : z);
}

bool triangle_contains(int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t cx, int64_t cy, int64_t x, int64_t y)
{
	int64_t det = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
	return  det * ((bx - ax) * (y - ay) - (by - ay) * (x - ax)) >= 0 &&
		det * ((cx - bx) * (y - by) - (cy - by) * (x - bx)) >= 0 &&
		det * ((ax - cx) * (y - cy) - (ay - cy) * (x - cx)) >= 0;
}

void draw_triangle(int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3, uint32_t colour)
{
	int64_t bx1 = min3(x1, x2, x3), by1 = min3(y1, y2, y3), bx2 = max3(x1, x2, x3), by2 = max3(y1, y2, y3);

	for (int64_t y = by1; y <= by2; ++y) {
		for (int64_t x = bx1; x <= bx2; ++x) {
			if (in_viewport(x, y) && triangle_contains(x1, y1, x2, y2, x3, y3, x, y)) {
				*((volatile uint32_t*)(framebuffer_address() + pixel_address(x, y))) = colour;
			}
		}
	}
}
