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

