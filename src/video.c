#include <kernel.h>
#include <flanterm.h>
#include <flanterm/fb.h>

static int64_t screen_x = 0, screen_y = 0, current_x = 0, current_y = 0, screen_graphics_x = 0, screen_graphics_y = 0;
static console first_console;
bool wait_state = false;
bool video_flip_is_auto = true;
bool video_dirty = true;
console* current_console = NULL;

extern volatile struct limine_module_request module_request;

volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST,
	.revision = 0,
};

static void *rr_fb_front = NULL;
static void *rr_fb_back = NULL;
static uint64_t rr_fb_pitch = 0;
static uint64_t rr_fb_height = 0;
static uint64_t rr_fb_bytes = 0;

struct flanterm_context *ft_ctx = NULL;

void rr_console_init_from_limine(void) {
	struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
	dprintf("fb front: %x\n", fb->address);
	rr_fb_front  = fb->address;
	rr_fb_pitch  = fb->pitch;
	rr_fb_height = fb->height;
	rr_fb_bytes  = rr_fb_pitch * rr_fb_height;  // full bytes, includes padding per row
	rr_fb_back = kmalloc(rr_fb_bytes);
	memset(rr_fb_back, 0, rr_fb_bytes);
}

inline uint64_t framebuffer_address()
{
	return (uint64_t)rr_fb_back;
}

inline uint64_t pixel_address(int64_t x, int64_t y)
{
	struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
	uint64_t pitch = fb->pitch;
	uint64_t bytes_per_pixel = fb->bpp >> 3;
	if (x >= 0 && y >= 0 && x < screen_graphics_x && y < screen_graphics_y) {
		return (y * pitch) + (x * bytes_per_pixel);
	}
	return 0;
}

uint64_t get_text_width()
{
	return screen_x;
}

uint64_t get_text_height()
{
	return screen_y;
}

void ft_write(struct flanterm_context *ctx, const char *buf, size_t count) {
	if (!ctx || !buf || !count) {
		return;
	}
	flanterm_write(ctx, buf, count);
	video_dirty = true;
}

void screenonly(console* c, const char* s)
{
	ft_write(ft_ctx, s, strlen(s));
}

void get_text_position(uint64_t* x, uint64_t* y)
{
	wait_state = true;
	putstring(current_console, "\033[6n");
	while (wait_state);
	*x = current_x;
	*y = current_y;
}

void gotoxy(uint64_t x, uint64_t y)
{
	char cursor_command[32];
	snprintf(cursor_command, 32, "\033[%d;%dH", y % (get_text_height() + 1), x % (get_text_width() + 1));
	screenonly(current_console, cursor_command);
	video_dirty = true;
}

void putpixel(int64_t x, int64_t y, uint32_t rgb)
{
	volatile uint32_t* addr = (volatile uint32_t*)(framebuffer_address() + pixel_address(x, y));
	*addr = rgb;
	video_dirty = true;
}

uint32_t getpixel(int64_t x, int64_t y)
{
	return *((volatile uint32_t*)(framebuffer_address() + pixel_address(x, y)));
}

/* Clear the screen - note this does not send the ansi to the debug console */
void clearscreen(console* c)
{
	memset(rr_fb_back, 0, rr_fb_bytes);
	screenonly(c, "\033[2J\033[0;0H");
	video_dirty = true;
}

void dput(const char n)
{
	outb(0xE9, n);
}

/* Write one character to the screen. As this calls setcursor() it may
 * trigger scrolling if the character would be off-screen.
 */
void put(console* c, const char n)
{
	dput(n);
	ft_write(ft_ctx, &n, 1);
}

void dputstring(const char* message)
{
	for (; *message; ++message) {
		outb(0xE9, *message);
		if (*message == 13) {
			outb(0xE9, 10);
		} else if (*message == 10) {
			outb(0xE9, 13);
		}
	}
}

/* Write a string to the screen. Most of the internals of this are
 * handled by put() and setcursor(), and the internal functions it
 * calls.
 */
void putstring(console* c, const char* message)
{
	dputstring(message);
	ft_write(ft_ctx, message, strlen(message));
}

void terminal_callback(struct flanterm_context *ctx, uint64_t type, uint64_t x, uint64_t y, uint64_t z)
{
	switch (type) {
		case FLANTERM_CB_BELL:
			beep(1000);
		break;
		case FLANTERM_CB_POS_REPORT:
			current_x = x - 1;
			current_y = y - 1;
		break;
	}
	wait_state = false;
}

void init_console()
{
	console* c = &first_console;
	current_console = c;
	c->internalbuffer = NULL;
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
		dprintf("No framebuffer offered\n");
		wait_forever();
	}
	rr_console_init_from_limine();

	struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];

	screen_graphics_x = fb->width;
	screen_graphics_y = fb->height;

	clearscreen(c);

	dprintf("Bringing up flanterm...\n");

	struct limine_module_response* mods = module_request.response;
	int mod_index = -1;
	void* font_data = NULL;
	if (mods && module_request.response->module_count) {
		for (size_t n = 0; n < mods->module_count; ++n) {
			if (!strcmp(mods->modules[n]->path, "/fonts/system.f16")) {
				mod_index = n;
			}
		}
		font_data = mods->modules[mod_index]->address;
		dprintf("Found font module: %x\n", font_data);
	}

	ft_ctx = flanterm_fb_init(
		NULL,
		NULL,
		rr_fb_back, fb->width, fb->height, fb->pitch,
		fb->red_mask_size, fb->red_mask_shift,
		fb->green_mask_size, fb->green_mask_shift,
		fb->blue_mask_size, fb->blue_mask_shift,
		NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		font_data,
		font_data ? 8 : 0,
		font_data ? 16 : 0,
		1,
		0,
		0,
		0
	);
	dprintf("Flanterm address: %x\n", ft_ctx);
	flanterm_set_autoflush(ft_ctx, true);
	flanterm_set_callback(ft_ctx, terminal_callback);
	size_t x, y;
	flanterm_get_dimensions(ft_ctx, &x, &y);
	screen_x = x;
	screen_y = y;
	dprintf("Framebuffer address: %llx x resolution=%d y resolution=%d\n", framebuffer_address(), screen_get_width(), screen_get_height());

	setforeground(current_console, COLOUR_LIGHTYELLOW);
	printf("Retro-Rocket ");
	setforeground(current_console, COLOUR_WHITE);
	printf("64-bit SMP kernel booting\n");
	rr_flip();

	init_debug();
}

int16_t screen_get_width()
{
	return screen_graphics_x;
}

int16_t screen_get_height()
{
	return screen_graphics_y;
}

/*
VGA

$00 Black
$01 Blue
$02 Green
$03 Cyan
$04 Red
$05 Magenta
$06 Brown
$07 White
$08 Gray
$09 Light Blue
$0A Light Green
$0B Light Cyan
$0C Light Red
$0D Light Magenta
$0E Yellow
$0F Bright White

ANSI

Black 	30
Red 	31
Green 	32
Yellow 	33
Blue 	34
Magenta 35
Cyan 	36
White 	37
*/
unsigned char map_vga_to_ansi(unsigned char colour)
{
	switch (colour) {
		case 0x00: // Black
			return 30;
		case 0x01: // Blue
			return 34;
		case 0x02: // Green
			return 32;
		case 0x03: // Cyan
			return 36;
		case 0x04: // Red
			return 31;
		case 0x05: // Magenta
			return 35;
		case 0x06: // Brown
			return 33;
		case 0x07: // White
			return 37;
		case 0x08: // Gray
			return 90;
		case 0x09: // Light Blue
			return 94;
		case 0x0A: // Light Green
			return 92;
		case 0x0B: // Light Cyan
			return 96;
		case 0x0C: // Light Red
			return 91;
		case 0x0D: // Light Magenta
			return 95;
		case 0x0E: // Yellow
			return 93;
		default: // Bright White
			return 97;
	}
}

void setbackground(console* c, unsigned char background)
{
	char code[100];
	snprintf(code, 100, "%c[%dm", 27, background + 40);
	putstring(c, code);
}

void setforeground(console* c, unsigned char foreground)
{
	char code[100];
	snprintf(code, 100, "%c[%dm", 27, map_vga_to_ansi(foreground));
	putstring(c, code);
}

bool video_flip_auto(void) {
	return video_flip_is_auto;
}

void set_video_auto_flip(bool flip) {
	video_flip_is_auto = flip;
	dprintf("VIDEO AUTO FLIP: %d\n", flip);
}

void rr_flip(void) {
	if (video_dirty) {
		memcpy(rr_fb_front, rr_fb_back, rr_fb_bytes);
		video_dirty = false;
	}
}