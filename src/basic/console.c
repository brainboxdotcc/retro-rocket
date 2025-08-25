/**
 * @file basic/console.c
 * @brief BASIC console IO functions
 */
#include <kernel.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

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
	return kinput(10240) == 0;
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
	const char* var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = proc_cur(logical_cpu_id());

	/* Clear buffer */
	if (kinput(MAX_STRINGLEN) == 0) {
		proc_set_idle(proc, check_input_in_progress, NULL);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	proc_set_idle(proc, NULL, NULL);

	switch (var[strlen(var) - 1]) {
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

void gotoxy_statement(struct basic_ctx* ctx)
{
	accept_or_return(CURSOR, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	gotoxy(x, y);
	accept_or_return(NEWLINE, ctx);
}

void print_statement(struct basic_ctx* ctx)
{
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

void colour_statement(struct basic_ctx* ctx, int tok)
{
	accept_or_return(tok, ctx);
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
	int no_newline = 0;
	int next_hex = 0;
	char buffer[MAX_STRINGLEN], out[MAX_STRINGLEN];
	
	*out = 0;

	do {
		no_newline = 0;
		*buffer = 0;
		if (tokenizer_token(ctx) == STRING) {
			if (tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
				snprintf(buffer, MAX_STRINGLEN, "%s", ctx->string);
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				tokenizer_error_print(ctx, "Unterminated \"");
				return NULL;
			}
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == COMMA) {
			strlcat(out, "\t", MAX_STRINGLEN);
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == SEMICOLON) {
			no_newline = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TILDE) {
			next_hex = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == PLUS) {
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == OPENBRACKET || tokenizer_token(ctx) == VARIABLE || tokenizer_token(ctx) == NUMBER || tokenizer_token(ctx) == HEXNUMBER) {
			/* Check if it's a string or numeric expression */
			const char* oldctx = ctx->ptr;
			if (tokenizer_token(ctx) != NUMBER && tokenizer_token(ctx) != HEXNUMBER && (*ctx->ptr == '"' || strchr(tokenizer_variable_name(ctx), '$'))) {
				ctx->ptr = oldctx;
				strlcpy(buffer, str_expr(ctx), MAX_STRINGLEN);
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				ctx->ptr = oldctx;
				const char* var_name = tokenizer_variable_name(ctx);
				ctx->ptr = oldctx;
				bool printable_double = (strchr(var_name, '#') != NULL || is_builtin_double_fn(var_name) || tokenizer_decimal_number(ctx));
				if (printable_double) {
					double f = 0.0;
					char double_buffer[32];
					ctx->ptr = oldctx;
					double_expr(ctx, &f);
					strlcat(buffer, double_to_string(f, double_buffer, 32, 0), MAX_STRINGLEN);
				} else {
					ctx->ptr = oldctx;
					snprintf(buffer, MAX_STRINGLEN, next_hex ? "%lX" : "%ld", expr(ctx));
				}
				strlcat(out, buffer, MAX_STRINGLEN);
				next_hex = 0;
			}
		} else {
			break;
		}
		numprints++;
	}
  	while(tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT && numprints < 255);
  
	if (!no_newline) {
		strlcat(out, "\n", MAX_STRINGLEN);
	}

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
	const char* var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = proc_cur(logical_cpu_id());

	if (!key_waiting()) {
		proc_set_idle(proc, check_key_waiting, NULL);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	proc_set_idle(proc, NULL, NULL);

	unsigned char c = kgetc();

	switch (var[strlen(var) - 1]) {
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
	return time(NULL) < proc->code->sleep_until;
}

void sleep_statement(struct basic_ctx* ctx)
{
	accept_or_return(SLEEP, ctx);
	int64_t sleep_length = expr(ctx);

	process_t* proc = proc_cur(logical_cpu_id());

	if (ctx->sleep_until == 0) {
		ctx->sleep_until = time(NULL) + sleep_length;
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
