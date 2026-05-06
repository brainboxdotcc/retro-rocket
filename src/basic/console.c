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
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* key_ascii = strval;
	PARAMS_END("INKEY$","");

	if (*key_ascii) {
		/*Checking for particular key being held */
		if (key_held(*key_ascii)) {
			char r_str[2] = {*key_ascii, 0};
			return (char*)gc_strdup(ctx, r_str);
		}
		return "";
	}

	const uint8_t key[2] = { kgetc(), 0 };
	
	if (*key == 255) {
		__builtin_ia32_pause();
		return "";
	} else {
		return (char*)gc_strdup(ctx, (const char*)key);
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
 * @param opaque BASIC context
 * @return true if input is still waiting for completion, false if done
 */
bool check_input_in_progress(process_t* proc, void* opaque)
{
	basic_ctx* ctx = opaque;
	if (basic_esc()) {
		return false;
	}
	return kinput(ctx, &ctx->input) == 0;
}

/**
 * @brief Deferred cooperative input via idle process callback.
 *
 * When a BASIC `INPUT` statement is encountered, the interpreter checks whether
 * a complete line of user input is available via `kinput(ctx, )`. If not, the process
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
	if (!var) {
		return;
	}

	process_t* proc = ctx->proc;

	/* Poll for pending input */
	if (kinput(ctx, &ctx->input) == 0) {
		proc_set_idle(proc, check_input_in_progress, ctx);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	proc_set_idle(proc, NULL, NULL);

	if (varname_is_int_array_access(ctx, var)) {
		int64_t index = arr_target_index(ctx);
		int64_t value = atoll(kgetinput(&ctx->input), 10);

		if (index == -1) {
			basic_set_int_array(var, value, ctx);
		} else {
			basic_set_int_array_variable(var, index, value, ctx);
		}

		kfreeinput(ctx, &ctx->input);
		accept_or_return(NEWLINE, ctx);
		proc->state = PROC_RUNNING;
		return;
	} else if (varname_is_string_array_access(ctx, var)) {
		int64_t index = arr_target_index(ctx);
		const char* value = kgetinput(&ctx->input);

		if (index == -1) {
			basic_set_string_array(var, value, ctx);
		} else {
			basic_set_string_array_variable(var, index, value, ctx);
		}

		kfreeinput(ctx, &ctx->input);
		accept_or_return(NEWLINE, ctx);
		proc->state = PROC_RUNNING;
		return;
	} else if (varname_is_double_array_access(ctx, var)) {
		int64_t index = arr_target_index(ctx);
		double value = 0;
		atof(kgetinput(&ctx->input), &value);

		if (index == -1) {
			basic_set_double_array(var, value, ctx);
		} else {
			basic_set_double_array_variable(var, index, value, ctx);
		}

		kfreeinput(ctx, &ctx->input);
		accept_or_return(NEWLINE, ctx);
		proc->state = PROC_RUNNING;
		return;
	}

	accept_or_return(VARIABLE, ctx);

	switch (var[var_length - 1]) {
		case '$':
			basic_set_string_variable(var, kgetinput(&ctx->input), ctx, false, false);
			break;

		case '#': {
			double f = 0;
			atof(kgetinput(&ctx->input), &f);
			basic_set_double_variable(var, f, ctx, false, false);
			break;
		}

		default:
			basic_set_int_variable(var, atoll(kgetinput(&ctx->input), 10), ctx, false, false);
			break;
	}
	kfreeinput(ctx, &ctx->input);
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
	double scale_x;
	double_expr(ctx, &scale_x);
	accept_or_return(COMMA, ctx);
	double scale_y;
	double_expr(ctx, &scale_y);
	accept_or_return(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		graphics_putstring(out, x, y, ctx->graphics_colour, scale_x, scale_y);
	}
}

void page_statement(struct basic_ctx* ctx) {
	accept_or_return(PAGE, ctx);
	enum token_t state = tokenizer_token(ctx);
	if (state == ON) {
		set_console_paging_enabled(true);
		accept_or_return(ON, ctx);
	} else if (state == OFF) {
		set_console_paging_enabled(false);
		accept_or_return(OFF, ctx);
	} else {
		tokenizer_error_print(ctx, "PAGE: Unexpected token, expected ON or OFF");
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
			/* Move forward one character */
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
			set_console_paging_enabled(true);
			break;
		case 15:
			set_console_paging_enabled(false);
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

static bool is_expression_start(int token)
{
	switch (token) {
		case VARIABLE:
		case NUMBER:
		case STRING:
		case OPENBRACKET:
		case MINUS:
		case PLUS:
			return true;
	}
	return false;
}

static bool printable_append(struct basic_ctx* ctx, char** out, size_t* used, size_t* cap, const char* text)
{
	size_t len = strlen(text);

	if (*used + len + 1 > *cap) {
		size_t new_cap = *cap;

		while (*used + len + 1 > new_cap) {
			new_cap *= 2;
		}

		char* new_out = buddy_realloc(ctx->allocator, *out, new_cap);
		if (!new_out) {
			tokenizer_error_print(ctx, "Out of memory");
			return false;
		}

		*out = new_out;
		*cap = new_cap;
	}

	memcpy(*out + *used, text, len + 1);
	*used += len;

	return true;
}

char* printable_syntax(struct basic_ctx* ctx)
{
	int numprints = 0;
	bool no_newline = false;
	bool next_hex = false;
	char buffer[64];

	size_t out_cap = MAX_STRINGLEN;
	size_t out_used = 0;
	char* out = buddy_malloc(ctx->allocator, out_cap);
	if (!out) {
		tokenizer_error_print(ctx, "Out of memory");
		return NULL;
	}

	*out = 0;

	while (true) {
		bool handled = false;
		*buffer = 0;

		if (tokenizer_token(ctx) == NEWLINE || tokenizer_token(ctx) == ENDOFINPUT) {
			break;
		}

		switch (tokenizer_token(ctx)) {
			case COMMA:
				if (!printable_append(ctx, &out, &out_used, &out_cap, "\t")) {
					buddy_free(ctx->allocator, out);
					return NULL;
				}
				tokenizer_next(ctx);
				handled = true;
				break;

			case SEMICOLON:
				tokenizer_next(ctx);
				if (tokenizer_token(ctx) == NEWLINE || tokenizer_token(ctx) == ENDOFINPUT) {
					no_newline = true;
				}
				handled = true;
				break;

			case TILDE:
				next_hex = true;
				tokenizer_next(ctx);
				handled = true;
				break;

			default: {
				if (!is_expression_start(tokenizer_token(ctx))) {
					tokenizer_error_print(ctx, "Syntax error");
					break;
				}

				up_value v = up_make_int(0);
				up_eval_value(ctx, &v);

				if (v.kind == UP_STR) {
					if (!printable_append(ctx, &out, &out_used, &out_cap, v.v.s ? v.v.s : "")) {
						buddy_free(ctx->allocator, out);
						return NULL;
					}
				} else if (v.kind == UP_REAL) {
					char dbuf[32];
					if (!printable_append(ctx, &out, &out_used, &out_cap,
							      double_to_string(v.v.r, dbuf, sizeof(dbuf), 0))) {
						buddy_free(ctx->allocator, out);
						return NULL;
					}
				} else {
					if (next_hex) {
						snprintf(buffer, sizeof(buffer), "%lX", (long)v.v.i);
					} else {
						snprintf(buffer, sizeof(buffer), "%ld", (long)v.v.i);
					}

					if (!printable_append(ctx, &out, &out_used, &out_cap, buffer)) {
						buddy_free(ctx->allocator, out);
						return NULL;
					}
				}

				next_hex = false;
				handled = true;
			}
				break;
		}

		if (!handled) {
			break;
		}

		numprints++;
		if (numprints >= 255) {
			break;
		}
	}

	if (!no_newline) {
		if (!printable_append(ctx, &out, &out_used, &out_cap, "\n")) {
			buddy_free(ctx->allocator, out);
			return NULL;
		}
	}

	tokenizer_next(ctx);

	if (ctx->errored) {
		buddy_free(ctx->allocator, out);
		return NULL;
	}

	const char* result = gc_strdup(ctx, out);
	buddy_free(ctx->allocator, out);
	return (char*)result;
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

	if (sleep_length == 0) {
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
