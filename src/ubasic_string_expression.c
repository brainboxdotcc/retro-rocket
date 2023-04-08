#include <kernel.h>

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"')
	{
		if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
			return "";
		}
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_STRING, ctx);
		return ctx->string;
	}
	else
	{
		r = ubasic_get_string_variable(tokenizer_variable_name(ctx), ctx);
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

const char* str_factor(struct ubasic_ctx* ctx)
{
	const char* r;

	switch(tokenizer_token(ctx))
	{
		case TOKENIZER_LEFTPAREN:
			accept(TOKENIZER_LEFTPAREN, ctx);
			r = str_expr(ctx);
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			r = str_varfactor(ctx);
		break;
	}

	return r;
}

int str_relation(struct ubasic_ctx* ctx)
{
	int op, r;

	const char* r1 = str_expr(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ)
	{
		tokenizer_next(ctx);
		const char* r2 = str_expr(ctx);

		switch (op)
		{
			case TOKENIZER_LT:
				r = (strcmp(r1, r2) < 0);
			break;
			case TOKENIZER_GT:
				r = (strcmp(r1, r2) > 0);
			break;
			case TOKENIZER_EQ:
				r = (strcmp(r1, r2) == 0);
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r;
}

const char* str_expr(struct ubasic_ctx* ctx)
{
	char* t1;
	char* t2;
	char tmp[1024];
	int op;

	t1 = (char*)str_factor(ctx);
	op = tokenizer_token(ctx);

	strlcpy(tmp, t1, 1024);
	
	while (op == TOKENIZER_PLUS)
	{
		tokenizer_next(ctx);
		t2 = (char*)str_factor(ctx);
		switch (op)
		{
			case TOKENIZER_PLUS:
				strlcat(tmp, t2, 1024);
			break;
		}
		op = tokenizer_token(ctx);
	}
	return gc_strdup(tmp);
}
