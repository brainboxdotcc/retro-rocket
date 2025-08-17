#include <kernel.h>
#include <installer.h>

uint8_t numeric_choice(uint8_t min, uint8_t max) {
	unsigned char selection;
	do {
		while ((selection = kgetc(current_console)) == 255) { __asm__("hlt"); };
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
		put(current_console, ' ');
	}
	kprintf("%s", buffer);
}

void warning(const char* warning_message) {
	size_t len = strlen(warning_message);
	char empty_line[len + 4 + 1];
	memset(empty_line, ' ', sizeof(empty_line));
	empty_line[len + 4] = 0;
	centre_text("%s%s%s%s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, empty_line, VGA_RESET);
	centre_text("%s%s  %s  %s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, warning_message, VGA_RESET);
	centre_text("%s%s%s%s\n", VGA_FG_LIGHTWHITE, VGA_BG_LIGHTRED, empty_line, VGA_RESET);
}

