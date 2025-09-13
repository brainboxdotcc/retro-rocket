#include <kernel.h>

extern bool debug;

void statement(struct basic_ctx* ctx)
{
	enum token_t token = tokenizer_token(ctx);

	basic_debug("line %ld statement(%d)\n", ctx->current_linenum, token);

	GENERATE_DISPATCH_TABLE(keyword_dispatch);
	for (size_t t = 0; t < DISPATCH_TABLE_COUNT(keyword_dispatch); ++t) {
		if (token == keyword_dispatch[t].token) {
			if (keyword_dispatch[t].handler != NULL) {
				keyword_dispatch[t].handler(ctx);
				return;
			}
		}
	}

	/* If we got here, token wasn’t found in the table at all */
	dprintf("Unknown keyword: %d\n", token);
	return tokenizer_error_print(ctx, "Unknown keyword");
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
