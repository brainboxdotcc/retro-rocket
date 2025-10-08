/**
 * @file basic/console.c
 * @brief BASIC console IO functions
 */
#include <kernel.h>
#include "basic/unified_expression.h"

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

extern bool debug;

int64_t basic_get_text_max_x(struct basic_ctx* ctx)
{
	return get_text_width();
}

int64_t basic_get_text_max_y(struct basic_ctx* ctx)
{
	return get_text_height();
}

int64_t basic_get_text_cur_x(struct basic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return x;
}

int64_t basic_get_text_cur_y(struct basic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return y;
}

char* basic_inkey(struct basic_ctx* ctx)
{
	const uint8_t key[2] = { kgetc(), 0 };
	
	if (*key == 255) {
		_mm_pause();
		return "";
	} else {
		return gc_strdup(ctx, (const char*)key);
	}
}

int64_t basic_ctrlkey(struct basic_ctx* ctx)
{
	return ctrl_held();
}

int64_t basic_shiftkey(struct basic_ctx* ctx)
{
	return shift_held();
}

int64_t basic_altkey(struct basic_ctx* ctx)
{
	return alt_held();
}

int64_t basic_capslock(struct basic_ctx* ctx)
{
	return caps_lock_on();
}

/**
 * Check if input is complete yet
 * @param proc process
 * @return true if input is still waiting for completion, false if done
 */
bool check_input_in_progress(process_t* proc, void* opaque)
{
	if (basic_esc()) {
		return false;
	}
	return kinput(MAX_STRINGLEN) == 0;
}

/**
 * @brief Deferred cooperative input via idle process callback.
 *
 * When a BASIC `INPUT` statement is encountered, the interpreter checks whether
 * a complete line of user input is available via `kinput()`. If not, the process
 * voluntarily yields using `proc_set_idle()` and assigns a custom callback
 * (`check_input_in_progress`) to periodically test for input readiness.
 *
 * While idle, the process enters `PROC_IO_BOUND` state. Once the callback
 * returns `false`, the scheduler resumes execution at the original line
 * (via `jump_linenum()`), and input is finalised and assigned to the variable.
 *
 * This mechanism ensures non-blocking interactive IO without requiring preemption,
 * and keeps the interpreter logic readable and non-reentrant.
 */
void input_statement(struct basic_ctx* ctx)
{
	accept_or_return(INPUT, ctx);
	size_t var_length;
	const char* var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = ctx->proc;

	/* Clear buffer */
	if (kinput(MAX_STRINGLEN) == 0) {
		proc_set_idle(proc, check_input_in_progress, NULL);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	proc_set_idle(proc, NULL, NULL);

	switch (var[var_length - 1]) {
		case '$':
			basic_set_string_variable(var, kgetinput(), ctx, false, false);
		break;
		case '#': {
			double f = 0;
			atof(kgetinput(), &f);
			basic_set_double_variable(var, f, ctx, false, false);
		}
		break;
		default:
			basic_set_int_variable(var, atoll(kgetinput(), 10), ctx, false, false);
		break;
	}
	kfreeinput();
	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}

void cls_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLS, ctx);
	clearscreen();
	accept_or_return(NEWLINE, ctx);
}

void gotoxy_statement(struct basic_ctx* ctx) {
	accept_or_return(CURSOR, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	gotoxy(x, y);
	accept_or_return(NEWLINE, ctx);
}

void scrollregion_statement(struct basic_ctx* ctx) {
	accept_or_return(SCROLLREGION, ctx);
	int64_t top = expr(ctx) * 8;
	accept_or_return(COMMA, ctx);
	int64_t bottom = expr(ctx) * 8 + 8;
	add_scrollable(top, bottom);
	accept_or_return(NEWLINE, ctx);
}

void print_statement(struct basic_ctx* ctx) {
	accept_or_return(PRINT, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		uint64_t flags;
		lock_spinlock_irq(&console_spinlock, &flags);
		lock_spinlock(&debug_console_spinlock);
		putstring(out);
		unlock_spinlock(&debug_console_spinlock);
		unlock_spinlock_irq(&console_spinlock, flags);
	}
}

void graphprint_statement(struct basic_ctx* ctx) {
	accept_or_return(GRAPHPRINT, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	accept_or_return(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		graphics_putstring(out, x, y, ctx->graphics_colour);
	}
}

void vdu_statement(struct basic_ctx* ctx) {
	uint64_t current_x, current_y;
	get_text_position(&current_x, &current_y);
	accept_or_return(VDU, ctx);
	int64_t primary = expr(ctx);
	switch (primary) {
		case 0:
			/* Intentional non-op */
		case 1:
			/* Next character to printer only - unsupported */
		case 2:
			/* Printer enable - unsupported */
		case 3:
			/* Printer disable - unsupported */
		case 4:
			/* Text to text cursor position - unsupported */
		case 5:
			/* Text to graphics cursor position - unsupported */
		case 6:
			/* Enable text output to screen */
			break;
		case 7:
			beep(1000);
			break;
		case 8: {
			/* Move back one character */
			if (current_x == 0) {
				current_x = get_text_width() - 1;
				if (current_y > 0) {
					current_y--;
				}
			} else {
				current_x--;
			}
			gotoxy(current_x, current_y);
			break;
		}
		case 9: {
			/* Move back one character */
			if (current_x == get_text_width() - 1) {
				put(' '); // Forces scrolling
				current_x = 0;
				current_y++;
			} else {
				current_x++;
			}
			gotoxy(current_x, current_y);
			break;
		}
		case 10: {
			/* Move down one line */
			if (current_y == get_text_height() - 1) {
				put('\n');
			} else {
				current_y++;
				gotoxy(current_x, current_y);
			}
			break;
		}
		case 11: {
			/* Move up one line */
			if (current_y != 0) {
				current_y--;
				gotoxy(current_x, current_y);
			}
			break;
		}
		case 12: // CLS
		case 16: // CLG
			clearscreen();
			break;
		case 17: {
			accept_or_return(COMMA, ctx);
			setforeground(expr(ctx));
			break;
		}
		case 13:
			/* Move to start of line */
			current_x = 0;
			gotoxy(current_x, current_y);
			break;
		case 14:
			/* Auto paging mode on - unsupported */
			break;
		case 15:
			/* Auto paging mode off - unsupported */
			break;
		case 18: {
			/* Set graphics colour */
			accept_or_return(COMMA, ctx);
			ctx->graphics_colour = expr(ctx);
			break;
		}
		case 19:
			/* Set colour pallete - unsupported */
			break;
		case 20:
			/* Reset to default graphics colour */
			ctx->graphics_colour = 0xffffff;
			setforeground(COLOUR_WHITE);
			break;
		case 21:
			/* Disable VDU until VDU 6 - unsupported */
			break;
		case 22:
			/* Switch video modes - unsupported */
			break;
		case 23:
			/* Redefine character code */
			accept_or_return(COMMA, ctx);
			uint8_t character = (uint8_t) expr(ctx) & 0xFF;
			if (character >= 32) {
				uint8_t definition[8];
				for (int horizontal = 0; horizontal < 8; ++horizontal) {
					accept_or_return(COMMA, ctx);
					definition[horizontal] = expr(ctx) & 0xFF;
				}
				redefine_character(character, definition);
			} else {
				switch (character) {
					case 0: {
						/* Set text cursor width - unsupported */
						break;
					}
					case 1: {
						/* Disable or enable text cursor - unsupported */
						accept_or_return(COMMA, ctx);
						uint8_t enable = (uint8_t) expr(ctx) & 0xFF;
						putstring(enable ? "\033[?25h" : "\033[?25l");
						break;
					}
					case 7: {
						/* Scroll text viewport - unsupported */
						break;
					}
					case 16: {
						/* Cursor movement defaults - unsupported */
						break;
					}
					default:
						break;
				}
			}
			break;
		case 24:
			/* Set graphics viewport - unsupported */
			break;
		case 25:
			/* Draw line */
			ctx->current_token = LINE;
			draw_line_statement(ctx);
			break;
		case 26:
			/* Reset graphics and text viewports */
			putstring("\x1b[r");
			gotoxy(0,0);
			break;
		case 27:
			/* Write single character */
			put(expr(ctx));
			break;
		case 28:
			/* Define text viewport - only support top and bottom */
			accept_or_return(COMMA, ctx);
			int64_t x1 = expr(ctx);      /* parsed but ignored */
			accept_or_return(COMMA, ctx);
			int64_t y1 = expr(ctx);
			accept_or_return(COMMA, ctx);
			(void)expr(ctx);      /* parsed but ignored */
			accept_or_return(COMMA, ctx);
			int64_t y2 = expr(ctx);
			if (y1 < 0) {
				y1 = 0;
			}
			if (y2 >= (int64_t)get_text_height()) {
				y2 = get_text_height()-1;
			}
			if (y2 < y1) {
				int64_t t = y1;
				y1 = y2;
				y2 = t;
			}
			kprintf("\x1b[%lu;%lur", y1 + 1, y2 + 1);
			gotoxy(x1, y1);
			break;
		case 29:
			/* Set graphics origin - unsupported */
			break;
		case 30:
			/* Home cursor */
			gotoxy(0, 0);
			break;
		case 31:
			/* Move to position */
			gotoxy(expr(ctx), expr(ctx));
			break;
		case 127:
			/* Backspace-delete */
			put(8);
			break;
	}
	while (tokenizer_token(ctx) == COMMA || tokenizer_token(ctx) == SEMICOLON) {
		if (tokenizer_token(ctx) == SEMICOLON) {
			tokenizer_next(ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		tokenizer_next(ctx);
		(void)expr(ctx); // Discard trailing numbers
	}
	accept_or_return(NEWLINE, ctx);
}

void colour_statement(struct basic_ctx* ctx)
{
	if (tokenizer_token(ctx) != COLOR && tokenizer_token(ctx) != COLOUR) {
		tokenizer_error_print(ctx, "COLOUR expected");
		return;
	}
	tokenizer_next(ctx);
	setforeground(expr(ctx));
	accept_or_return(NEWLINE, ctx);
}

void background_statement(struct basic_ctx* ctx)
{
	accept_or_return(BACKGROUND, ctx);
	setbackground(expr(ctx));
	accept_or_return(NEWLINE, ctx);
}

char* printable_syntax(struct basic_ctx* ctx)
{
	int numprints = 0;
	bool no_newline = false;
	bool next_hex = false;
	char buffer[MAX_STRINGLEN], out[MAX_STRINGLEN];

	*out = 0;

	do {
		bool handled = false;
		no_newline = false;
		*buffer = 0;

		switch (tokenizer_token(ctx)) {
			case COMMA:
				strlcat(out, "\t", MAX_STRINGLEN);
				tokenizer_next(ctx);
				handled = true;
				break;

			case SEMICOLON:
				no_newline = 1;
				tokenizer_next(ctx);
				handled = true;
				break;

			case TILDE: /* next value in hex */
				next_hex = 1;
				tokenizer_next(ctx);
				handled = true;
				break;

			default: {
				/* Expression (string or numeric) - let the unified parser decide. */
				up_value v = up_make_int(0);
				up_eval_value(ctx, &v);  /* <- unified typed evaluator */

				if (v.kind == UP_STR) {
					/* Strings: append as-is */
					strlcat(out, v.v.s ? v.v.s : "", MAX_STRINGLEN);
				} else if (v.kind == UP_REAL) {
					/* Reals: standard formatter */
					char dbuf[32];
					strlcat(buffer, double_to_string(v.v.r, dbuf, sizeof dbuf, 0), MAX_STRINGLEN);
					strlcat(out, buffer, MAX_STRINGLEN);
				} else { /* UP_INT */
					/* Integers: optional hex via ~, otherwise decimal */
					if (next_hex) {
						snprintf(buffer, MAX_STRINGLEN, "%lX", (long)v.v.i);
					} else {
						snprintf(buffer, MAX_STRINGLEN, "%ld", (long)v.v.i);
					}
					strlcat(out, buffer, MAX_STRINGLEN);
				}
				next_hex = false;
				handled = true;
			}
			break;
		}

		if (!handled) {
			/* No valid argument/token to process. */
			break;
		}

		numprints++;
	} while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT && numprints < 255);

	if (!no_newline) {
		strlcat(out, "\n", MAX_STRINGLEN);
	}

	/* consume line end or EOF sentinel */
	tokenizer_next(ctx);

	if (ctx->errored) {
		return NULL;
	}
	return gc_strdup(ctx, out);
}

void keymap_statement(struct basic_ctx* ctx)
{
	accept_or_return(KEYMAP, ctx);
	const char* filename = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);

	char path[1024];
	snprintf(path, sizeof(path), "/system/keymaps/%s.keymap", filename);

	fs_directory_entry_t* fsi = fs_get_file_info(path);
	if (!fsi || fsi->flags & FS_DIRECTORY) {
		tokenizer_error_printf(ctx, "Keymap file '%s' not found", path);
		return;
	}

	unsigned char* keymap = buddy_malloc(ctx->allocator, fsi->size + 1);
	if (!keymap) {
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}
	keymap[fsi->size] = 0;

	if (fs_read_file(fsi, 0, fsi->size, keymap)) {
		load_keymap_from_string((const char*)keymap);
	} else {
		tokenizer_error_printf(ctx, "Failed to read keymap file '%s'", path);
	}

	buddy_free(ctx->allocator, keymap);
	keymap = NULL;
}

/**
 * Check if a key is waiting
 * @param proc Process
 * @return true if no key is ready, false otherwise
 */
bool check_key_waiting(process_t* proc, void* opaque)
{
	return !key_waiting(); // Return true if still waiting
}

/**
 * @brief Process GET statement.
 *
 * The GET statement waits for a keypress, then stores it into
 * a variable. It behaves like INPUT but only reads a single
 * character (without requiring ENTER).
 *
 * It yields execution while no key is pressed, using the
 * cooperative multitasking system to prevent blocking.
 *
 * @param ctx BASIC context
 */
void kget_statement(struct basic_ctx* ctx)
{
	accept_or_return(KGET, ctx);
	size_t var_length;
	const char* var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = ctx->proc;

	if (!key_waiting()) {
		proc_set_idle(proc, check_key_waiting, NULL);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	proc_set_idle(proc, NULL, NULL);

	unsigned char c = kgetc();

	switch (var[var_length - 1]) {
		case '$': {
			char str[2] = { c, '\0' };
			basic_set_string_variable(var, str, ctx, false, false);
			break;
		}
		case '#': {
			double f = (double)c;
			basic_set_double_variable(var, f, ctx, false, false);
			break;
		}
		default:
			basic_set_int_variable(var, (int)c, ctx, false, false);
			break;
	}

	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}

bool check_sleep_in_progress(process_t* proc, [[maybe_unused]] void* opaque)
{
	if (basic_esc()) {
		return false;
	}
	return get_ticks() < proc->code->sleep_until;
}

void sleep_statement(struct basic_ctx* ctx)
{
	accept_or_return(SLEEP, ctx);
	int64_t sleep_length = expr(ctx);

	process_t* proc = ctx->proc;

	if (sleep_length < 0 || sleep_length > 604800000) {
		tokenizer_error_print(ctx, "Invalid SLEEP duration");
		return;
	}

	if (ctx->sleep_until == 0) {
		ctx->sleep_until = get_ticks() + sleep_length;
		proc_set_idle(proc, check_sleep_in_progress, NULL);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_SUSPENDED;
		return;
	}

	proc_set_idle(proc, NULL, NULL);
	ctx->sleep_until = 0;

	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}
