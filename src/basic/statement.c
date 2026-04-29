#include <kernel.h>

extern bool debug;

GENERATE_HANDLER_TABLE(dispatch_by_token)

void statement(struct basic_ctx* ctx)
{
	GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
	GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)
	enum token_t token = tokenizer_token(ctx);
	if (dispatch_by_token[token] != NULL) {
		if (is_restricted_len(ctx, token_names[token], token_name_lengths[token])) {
			tokenizer_error_printf(ctx, "Keyword '%s' is restricted by parent program", token_names[token]);
			return;
		}
		dispatch_by_token[token](ctx);
		return;
	} else if (token != NO_TOKEN) {
		tokenizer_error_printf(ctx, "Keyword %s can't be used here", token_names[token]);
		return;
	}

	dprintf("Unknown keyword: %d\n", token);
	tokenizer_error_print(ctx, "Unknown keyword");
}

void line_statement(struct basic_ctx* ctx)
{
	if (tokenizer_token(ctx) == NEWLINE) {
		/* Empty line! */
		accept(NEWLINE, ctx);
		return;
	}
	int64_t line = tokenizer_num(ctx, NUMBER);
	if (line == 0) {
		return tokenizer_error_printf(ctx, "Missing line number after line %lu: %s", ctx->current_linenum, ctx->ptr);
	}
	ctx->current_linenum = line;
	accept_or_return(NUMBER, ctx);
	statement(ctx);
}
