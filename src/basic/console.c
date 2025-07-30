/**
 * @file basic/console.c
 * @brief BASIC console IO functions
 */
#include <kernel.h>

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
	const uint8_t key[2] = { kgetc((console*)ctx->cons), 0 };
	
	if (*key == 255) {
		// hlt stops busy waiting for INKEY$
		__asm__ volatile("hlt");
		return "";
	} else {
		return gc_strdup((const char*)key);
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
 * @brief Process INPUT statement.
 * 
 * The INPUT statement will yield while waiting for input, essentially if
 * there is no complete line in the input buffer yet, it will yield back to
 * the OS task loop for other processes to get a turn. Each time the process
 * is entered while waiting for the input line to be completed, it will just
 * loop back against the same INPUT statement again until completed.
 * 
 * @param ctx BASIC context
 */
void input_statement(struct basic_ctx* ctx)
{
	accept_or_return(INPUT, ctx);
	const char* var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	/* Clear buffer */
	if (kinput(10240, (console*)ctx->cons) != 0) {
		switch (var[strlen(var) - 1]) {
			case '$':
				basic_set_string_variable(var, kgetinput((console*)ctx->cons), ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(kgetinput((console*)ctx->cons), &f);
				basic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				basic_set_int_variable(var, atoll(kgetinput((console*)ctx->cons), 10), ctx, false, false);
			break;
		}
		kfreeinput((console*)ctx->cons);
		accept_or_return(NEWLINE, ctx);
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

void cls_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLS, ctx);
	clearscreen(current_console);
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
		putstring((console*)ctx->cons, out);
	}
}

void colour_statement(struct basic_ctx* ctx, int tok)
{
	accept_or_return(tok, ctx);
	setforeground((console*)ctx->cons, expr(ctx));
	accept_or_return(NEWLINE, ctx);
}

void background_statement(struct basic_ctx* ctx)
{
	accept_or_return(BACKGROUND, ctx);
	setbackground((console*)ctx->cons, expr(ctx));
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
	return gc_strdup(out);
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
		tokenizer_error_print(ctx, "Keymap file not found");
		return;
	}

	unsigned char* keymap = kmalloc(fsi->size + 1);
	if (!keymap) {
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}
	keymap[fsi->size] = 0;

	if (fs_read_file(fsi, 0, fsi->size, keymap)) {
		load_keymap_from_string((const char*)keymap);
	} else {
		tokenizer_error_print(ctx, "Failed to read keymap file");
	}

	kfree_null(&keymap);
}