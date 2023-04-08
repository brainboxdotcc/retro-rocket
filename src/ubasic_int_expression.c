#include <kernel.h>

static int64_t varfactor(struct ubasic_ctx* ctx)
{
	int64_t r = ubasic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	// Special case for builin functions
	if (tokenizer_token(ctx) == TOKENIZER_COMMA)
		tokenizer_error_print(ctx, "Too many parameters for builtin function");
	else
	{
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

int64_t factor(struct ubasic_ctx* ctx)
{
	int64_t r = 0;

	int tok = tokenizer_token(ctx);
	switch (tok) {
		case TOKENIZER_NUMBER:
		case TOKENIZER_HEXNUMBER:
			r = tokenizer_num(ctx, tok);
			accept(tok, ctx);
		break;
		case TOKENIZER_LEFTPAREN:
			accept(TOKENIZER_LEFTPAREN, ctx);
			r = expr(ctx);
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			r = varfactor(ctx);
		break;
	}
	return r;
}

int relation(struct ubasic_ctx* ctx)
{
	int r1, r2;
	int op;

	r1 = expr(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ)
	{
		tokenizer_next(ctx);
		r2 = expr(ctx);

		switch (op)
		{
			case TOKENIZER_LT:
				r1 = r1 < r2;
			break;
			case TOKENIZER_GT:
				r1 = r1 > r2;
			break;
			case TOKENIZER_EQ:
				r1 = r1 == r2;
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r1;
}

int64_t term(struct ubasic_ctx* ctx)
{
	int64_t f1, f2;
	int op;

	f1 = factor(ctx);
	//kprintf("Factor is %d\n", f1);
	op = tokenizer_token(ctx);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD)
	{
		tokenizer_next(ctx);
		f2 = factor(ctx);
		switch (op)
		{
			case TOKENIZER_ASTR:
				f1 = f1 * f2;
			break;
			case TOKENIZER_SLASH:
				if (f2 == 0) {
					tokenizer_error_print(ctx, "Division by zero");
				} else {
					f1 = f1 / f2;
				}
			break;
			case TOKENIZER_MOD:
				f1 = f1 % f2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	return f1;
}


int64_t expr(struct ubasic_ctx* ctx)
{
	int64_t t1, t2;
	int op;

	t1 = term(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR)
	{
		tokenizer_next(ctx);
		t2 = term(ctx);
		switch (op)
		{
			case TOKENIZER_PLUS:
				t1 = t1 + t2;
			break;
			case TOKENIZER_MINUS:
				t1 = t1 - t2;
			break;
			case TOKENIZER_AND:
				t1 = t1 & t2;
			break;
			case TOKENIZER_OR:
				t1 = t1 | t2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	return t1;
}
