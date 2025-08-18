#include <kernel.h>
#include <installer.h>

uint8_t numeric_choice(uint8_t min, uint8_t max) {
	unsigned char selection;
	do {
		while ((selection = kgetc()) == 255) { __asm__("hlt"); };
		selection -= '0';
	} while (selection < min || selection > max);
	kprintf("%c\n", selection + '0');
	return selection;
}

void centre_text(const char* format, ...) {
	static char buffer[MAX_STRINGLEN];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, MAX_STRINGLEN, format, args);
	va_end(args);
	size_t len = strlen_ansi(buffer);
	uint64_t centre = get_text_width() / 2;
	uint64_t left = centre - (len / 2);
	for (uint64_t x = 0; x < left; ++x) {
		put(' ');
	}
	kprintf("%s", buffer);
}

void warning(const char* warning_message, const char* subtitle) {
	size_t l1 = strlen_ansi(warning_message), l2 = subtitle ? strlen_ansi(subtitle) : 0;
	size_t len = l1 > l2 ? l1 : l2;
	char empty_line[len + 4 + 1];
	memset(empty_line, ' ', sizeof(empty_line));
	empty_line[len + 4] = 0;
	uint64_t left = (get_text_width() / 2) - (len / 2) - 4, w = len + 8, x, y;
	kprintf("\n");
	get_text_position(&x, &y);
	centre_text("%s%s%s%s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, empty_line, VGA_RESET);
	centre_text("%s%s  %s  %s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, warning_message, VGA_RESET);
	centre_text("%s%s%s%s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, empty_line, VGA_RESET);
	draw_box_cp437_double(left, y, w, 5);
	kprintf("\n");
}

void draw_box_cp437_double(uint64_t x, uint64_t y, uint64_t width, uint64_t height) {
	uint64_t cur_x;
	uint64_t cur_y;
	uint64_t screen_w;
	uint64_t screen_h;

	get_text_position(&cur_x, &cur_y);

	screen_w = get_text_width();
	screen_h = get_text_height();

	if (x >= screen_w || y >= screen_h) {
		return;
	}

	if (x + width > screen_w) {
		width = screen_w - x;
	}
	if (y + height > screen_h) {
		height = screen_h - y;
	}

	if (width < 2 || height < 2) {
		gotoxy(cur_x, cur_y);
		return;
	}

	/* Top edge */
	gotoxy(x, y);
	kprintf(GLYPH_FMT, CP437_DBL_TL);

	for (uint64_t i = 0; i < (width - 2); i++) {
		kprintf(GLYPH_FMT, CP437_DBL_H);
	}

	kprintf(GLYPH_FMT, CP437_DBL_TR);

	/* Sides */
	for (uint64_t row = 0; row < (height - 2); row++) {
		gotoxy(x, y + 1 + row);
		kprintf(GLYPH_FMT, CP437_DBL_V);

		gotoxy(x + width - 1, y + 1 + row);
		kprintf(GLYPH_FMT, CP437_DBL_V);
	}

	/* Bottom edge */
	gotoxy(x, y + height - 1);
	kprintf(GLYPH_FMT, CP437_DBL_BL);

	for (uint64_t i = 0; i < (width - 2); i++) {
		kprintf(GLYPH_FMT, CP437_DBL_H);
	}

	kprintf(GLYPH_FMT, CP437_DBL_BR);

	gotoxy(cur_x, cur_y);
}

void draw_box_cp437_double_center(uint64_t req_w, uint64_t req_h, uint64_t *out_x, uint64_t *out_y, uint64_t *out_w, uint64_t *out_h) {
	uint64_t screen_w = get_text_width();
	uint64_t screen_h = get_text_height();

	uint64_t width  = req_w;
	uint64_t height = req_h;

	if (width > screen_w) {
		width = screen_w;
	}
	if (height > screen_h) {
		height = screen_h;
	}

	uint64_t x = (screen_w - width) / 2;
	uint64_t y = (screen_h - height) / 2;

	draw_box_cp437_double(x, y, width, height);

	if (out_x != NULL) {
		*out_x = x;
	}
	if (out_y != NULL) {
		*out_y = y;
	}
	if (out_w != NULL) {
		*out_w = width;
	}
	if (out_h != NULL) {
		*out_h = height;
	}
}

void draw_progress_bar_cp437(uint64_t x, uint64_t y, uint64_t width, uint64_t percent) {
	uint64_t cur_x;
	uint64_t cur_y;
	uint64_t screen_w;
	uint64_t screen_h;
	uint64_t height = 3;

	get_text_position(&cur_x, &cur_y);

	screen_w = get_text_width();
	screen_h = get_text_height();

	if (x >= screen_w || y >= screen_h) {
		return;
	}

	/* Clip to screen */
	if (x + width > screen_w) {
		width = screen_w - x;
	}
	if (y + height > screen_h) {
		height = screen_h - y;
	}

	if (width < 3 || height < 3) {
		gotoxy(cur_x, cur_y);
		return;
	}

	/* Border */
	draw_box_cp437_double(x, y, width, height);

	/* Fill line calculations */
	if (percent > 100) {
		percent = 100;
	}

	uint64_t inner_w = width - 2;
	uint64_t total_half_cells = inner_w * 2;
	uint64_t filled_halves = (percent * total_half_cells) / 100;

	/* Draw fill (row y+1). Left→right with half-cell precision. */
	for (uint64_t i = 0; i < inner_w; i++) {
		uint64_t cell_x = x + 1 + i;
		gotoxy(cell_x, y + 1);

		if (filled_halves >= 2) {
			kprintf(GLYPH_FMT, CP437_BLOCK_FULL);
			filled_halves -= 2;
		} else if (filled_halves == 1) {
			kprintf(GLYPH_FMT, CP437_BLOCK_HALF_LEFT);
			filled_halves = 0;
		} else {
			kprintf(" ");
		}
	}

	gotoxy(cur_x, cur_y);
}

void draw_text_box_cp437(uint64_t x, uint64_t y, uint64_t width, const char *text) {
	uint64_t cur_x;
	uint64_t cur_y;
	uint64_t screen_w;
	uint64_t screen_h;
	uint64_t height = 3;

	get_text_position(&cur_x, &cur_y);

	screen_w = get_text_width();
	screen_h = get_text_height();

	if (x >= screen_w || y >= screen_h) {
		return;
	}

	if (x + width > screen_w) {
		width = screen_w - x;
	}
	if (y + height > screen_h) {
		height = screen_h - y;
	}

	if (width < 3 || height < 3) {
		gotoxy(cur_x, cur_y);
		return;
	}

	draw_box_cp437_double(x, y, width, height);

	uint64_t inner_w = width - 2;
	uint64_t text_len = 0;
	uint64_t i;

	if (text != NULL) {
		text_len = strlen_ansi(text);
	}

	gotoxy(x + 1, y + 1);

	if (inner_w == 0) {
		gotoxy(cur_x, cur_y);
		return;
	}

	if (text == NULL || text_len == 0) {
		for (i = 0; i < inner_w; i++) {
			kprintf(" ");
		}
		gotoxy(cur_x, cur_y);
		return;
	}

	if (text_len <= inner_w) {
		kprintf("%s", text);
		for (; i < inner_w; i++) {
			kprintf(" ");
		}
	} else {
		if (inner_w >= 3) {
			uint64_t keep = inner_w - 3;
			for (i = 0; i < keep; i++) {
				kprintf("%c", text[i]);
			}
			kprintf("...");
		} else {
			for (i = 0; i < inner_w; i++) {
				kprintf("%c", text[i]);
			}
		}
	}

	gotoxy(cur_x, cur_y);
}

void draw_text_box_cp437_center(uint64_t x, uint64_t y, uint64_t width, const char *text)
{
	if (text == NULL) {
		draw_text_box_cp437(x, y, width, NULL);
		return;
	}

	uint64_t inner_w = (width > 2) ? width - 2 : 0;
	uint64_t text_len = strlen_ansi(text);

	if (inner_w == 0 || text_len == 0 || text_len > inner_w) {
		/* falls back to normal behaviour with truncation/ellipsis */
		draw_text_box_cp437(x, y, width, text);
		return;
	}

	/* compute left padding */
	uint64_t pad_left = (inner_w - text_len) / 2;

	/* create a padded buffer */
	static char buf[MAX_STRINGLEN];
	uint64_t pos = 0;

	for (uint64_t i = 0; i < pad_left && pos < sizeof(buf) - 1; i++) {
		buf[pos++] = ' ';
	}
	for (uint64_t i = 0; i < text_len && pos < sizeof(buf) - 1; i++) {
		buf[pos++] = text[i];
	}

	buf[pos] = '\0';

	draw_text_box_cp437(x, y, width, buf);
}

void new_page(const char* title) {
	clearscreen();
	kprintf("\n\n\n");
	centre_text("%s%s%s\n\n", VGA_FG_YELLOW, title, VGA_RESET);
}