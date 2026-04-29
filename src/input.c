#include <kernel.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

bool kinput(struct basic_ctx* basic, struct buffered_input_context_t* ctx) {
	
	ctx->last = kgetc();

	if (ctx->stopped && ctx->buffer != NULL) {
		return true;
	}

	if (ctx->last == 255) {
		__asm__ volatile("hlt");
		return false;
	}

	if (ctx->internalbuffer == NULL) {
		ctx->stopped = false;
		ctx->buflen = 256;
		ctx->internalbuffer = buddy_malloc(basic->allocator, ctx->buflen);
		if (!ctx->internalbuffer) {
			return false;
		}
		ctx->buffer = ctx->internalbuffer;
		ctx->bufcnt = 0;
		ctx->internalbuffer[0] = 0;
	}

	bool need_grow = false;
	if (ctx->last != '\r' && ctx->last != 8 && ctx->last != KEY_UP && ctx->last != KEY_DOWN && ctx->last != KEY_LEFT && ctx->last != KEY_RIGHT) {
		need_grow = ctx->bufcnt + 1 >= ctx->buflen;
	}

	if (need_grow) {
		if (ctx->buflen > SIZE_MAX / 2) {
			beep(1000);
			return false;
		}

		size_t new_len = ctx->buflen * 2;
		char *new_buffer = buddy_realloc(basic->allocator, ctx->internalbuffer, new_len);
		if (!new_buffer) {
			beep(1000);
			return false;
		}
		ctx->internalbuffer = new_buffer;
		ctx->buffer = ctx->internalbuffer + ctx->bufcnt;
		ctx->buflen = new_len;
	}

	uint64_t flags;
	lock_spinlock_irq(&console_spinlock, &flags);
	lock_spinlock(&debug_console_spinlock);
	switch (ctx->last) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		break;
		case '\r':
			put('\n');
		break;
		case 8:
			if (ctx->bufcnt != 0) {
				ctx->buffer--;
				ctx->bufcnt--;
				/* Advance text cursor back one space, if we are at x=0, move to x=79, y--.
				 * If we are at y=0, scroll up?
				 */
				put(8);
				put(' ');
				put(8);
			} else {
				beep(1000);
			}
		break;
		default:
			*(ctx->buffer++) = ctx->last;
			ctx->bufcnt++;
			put(ctx->last);
		break;
	}
	unlock_spinlock(&debug_console_spinlock);
	unlock_spinlock_irq(&console_spinlock, flags);
	/* Terminate string */
	*ctx->buffer = 0;

	if (ctx->last == '\r') {
		ctx->stopped = true;
	}

	return ctx->last == '\r';
}

void kinitinput(struct buffered_input_context_t* ctx) {
	memset(ctx, 0, sizeof(buffered_input_context_t));
}

void kfreeinput(struct basic_ctx* basic, struct buffered_input_context_t* ctx) {
	buddy_free(basic->allocator, ctx->internalbuffer);
	kinitinput(ctx);
}

const char* kgetinput(struct buffered_input_context_t* ctx) {
	return ctx->internalbuffer;
}
