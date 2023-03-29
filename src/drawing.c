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