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
	/*if (from_y == to_y) {
		draw_horizontal_line(from_x, to_x, from_y, colour);
		return;
	}*/

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

/**
 * @brief Returns true if a coordinate is within the viewport
 * 
 * @param x point X coordinate
 * @param y point Y coordinate
 * @return true if point is within the viewport
 */
static bool in_viewport(int64_t x, int64_t y)
{
	return (x >= 0 && y >= 0 && x < screen_get_width() && y < screen_get_height());
}

/**
 * @brief Return the minimum of three integers
 * 
 * @param x first integer
 * @param y second integer
 * @param z third intefer
 * @return int64_t smallest value
 */
static int64_t min3(int64_t x, int64_t y, int64_t z)
{
	return (x < ((y < z) ? y : z) ) ? x : ((y < z) ? y : z);
}

/**
 * @brief Return the maximum of three integers
 * 
 * @param x first integer
 * @param y second integer
 * @param z third integer
 * @return int64_t largest value
 */
static int64_t max3(int64_t x, int64_t y, int64_t z)
{
	return (x > ((y > z) ? y : z) ) ? x : ((y > z) ? y : z);
}

/**
 * @brief Line intersection algorithm to determine if a point is within a polygon
 * 
 * @param ax polygon first coordinate x
 * @param ay polygon first coordinate y
 * @param bx polygon second coordinate x
 * @param by polygon second coordinate y
 * @param cx polygon third coordinate x
 * @param cy polygon third coordinate y
 * @param x point x
 * @param y point y
 * @return true if point is within the three sided polygon
 */
static bool triangle_contains(int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t cx, int64_t cy, int64_t x, int64_t y)
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


/**
 * @brief Draw part of a chord, 1/8th of a circle, and duplicate it eight times
 * 
 * @param xc X centre coordinate
 * @param yc Y centre coordinate
 * @param x X position of edge
 * @param y Y position of edge
 * @param fill True to fill, render using triangles
 * @param colour Colour of chord
 */
void draw_chord(int64_t xc, int64_t yc, int64_t x, int64_t y, bool fill, uint32_t colour)
{
	if (fill) {
		draw_horizontal_line(xc - x, xc + x, yc + y, colour);
		draw_horizontal_line(xc - x, xc + x, yc - y, colour);
		draw_horizontal_line(xc - y, xc + y, yc + x, colour);
		draw_horizontal_line(xc - y, xc + y, yc - x, colour);
		return;
	}
	putpixel(xc + x, yc + y, colour);
	putpixel(xc - x, yc + y, colour);
	putpixel(xc + x, yc - y, colour);
	putpixel(xc - x, yc - y, colour);
	putpixel(xc + y, yc + x, colour);
	putpixel(xc - y, yc + x, colour);
	putpixel(xc + y, yc - x, colour);
	putpixel(xc - y, yc - x, colour);
}

void draw_circle(int64_t x_centre, int64_t y_centre, int64_t radius, bool fill, uint32_t colour)
{
	int64_t x = 0, y = radius;
	int64_t delta = 3 - 2 * radius;
	draw_chord(x_centre, y_centre, x, y, fill, colour);
	while (y >= x) {
		x++;
		if (delta > 0) {
			y--; 
			delta += 4 * (x - y) + 10;
		} else {
			delta += 4 * x + 6;
		}
		draw_chord(x_centre, y_centre, x, y, fill, colour);
	}
}