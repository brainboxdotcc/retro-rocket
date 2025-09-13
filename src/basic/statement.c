#include <kernel.h>

extern bool debug;

void statement(struct basic_ctx* ctx)
{
	enum token_t token = tokenizer_token(ctx);

	basic_debug("line %ld statement(%d)\n", ctx->current_linenum, token);

	GENERATE_HANDLER_TABLE(dispatch_by_token)
	if (token < DISPATCH_TABLE_COUNT(dispatch_by_token)) {
		if (dispatch_by_token[token] != NULL) {
			dispatch_by_token[token](ctx);
			return;
		}
	}

	dprintf("Unknown keyword: %d\n", token);
	tokenizer_error_print(ctx, "Unknown keyword");

}

void line_statement(struct basic_ctx* ctx)
{
	basic_debug("line_statement\n");
	if (tokenizer_token(ctx) == NEWLINE) {
		/* Empty line! */
		accept(NEWLINE, ctx);
		return;
	}
	int64_t line = tokenizer_num(ctx, NUMBER);
	basic_debug("line_statement parsed line %ld\n", line);
	if (line == 0) {
		return tokenizer_error_printf(ctx, "Missing line number after line %lu", ctx->current_linenum);
	}
	ctx->current_linenum = line;
	accept_or_return(NUMBER, ctx);
	statement(ctx);
}
