#include <kernel.h>

void double_varfactor(struct ubasic_ctx* ctx, double* res)
{
	double r;

	ubasic_get_numeric_variable(tokenizer_variable_name(ctx), ctx, &r);

	//dprintf("double_varfactor\n");

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
	*res = r;
}

void double_factor(struct ubasic_ctx* ctx, double* res)
{
	//char buffer[50];

	int tok = tokenizer_token(ctx);
	switch (tok) {
		case TOKENIZER_NUMBER:
			//dprintf("double_factor TOKENIZER_NUMBER\n");
			tokenizer_fnum(ctx, tok, res);
			//dprintf("double_factor fnum->r=%s\n", double_to_string(*res, buffer, 50, 0));
			accept(tok, ctx);
		break;
		case TOKENIZER_LEFTPAREN:
			//dprintf("double_factor TOKENIZER_LEFTPAREN\n");
			accept(TOKENIZER_LEFTPAREN, ctx);
			double_expr(ctx, res);
			//dprintf("double_factor expr->r=%s\n", double_to_string(*res, buffer, 50, 0));
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			//dprintf("double_factor default\n");
			double_varfactor(ctx, res);
			//dprintf("double_factor varfactor->r=%s\n", double_to_string(*res, buffer, 50, 0));
		break;
	}
	//dprintf("double_factor end, r=%s\n", double_to_string(*res, buffer, 50, 0));
}

void double_term(struct ubasic_ctx* ctx, double* res)
{
	double f1, f2;
	int op;
	//char buffer[50];

	//dprintf("double_term first double_factor call\n");
	double_factor(ctx, &f1);
	//dprintf("double_term f1=%s\n", double_to_string(f1, buffer, 50, 0));

	op = tokenizer_token(ctx);
	//dprintf("first op=%d %s\n", op, types[op]);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD)
	{
		tokenizer_next(ctx);
		//dprintf("double_term second double_factor call\n");
		double_factor(ctx, &f2);
		//dprintf("double_term f2=%s\n", double_to_string(f2, buffer, 50, 0));
		switch (op)
		{
			case TOKENIZER_ASTR:
				f1 = f1 * f2;
			break;
			case TOKENIZER_SLASH:
				if (f2 == 0.0) {
					tokenizer_error_print(ctx, "Division by zero");
					*res = 0.0;
				} else {
					f1 = f1 / f2;
				}
			break;
			case TOKENIZER_MOD:
				f1 = (int64_t)f1 % (int64_t)f2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	//dprintf("final op=%d %s\n", op, types[op]);
	//dprintf("double_term returning %s\n", double_to_string(f1, buffer, 50, 0));
	*res = f1;
}

void double_expr(struct ubasic_ctx* ctx, double* res)
{
	double t1, t2;
	int op;

	//dprintf("double_expr()\n");

	double_term(ctx, &t1);
	op = tokenizer_token(ctx);
	//dprintf("double_expr before type, type is %d %s\n", op, types[op]);

	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR) {
		//dprintf("double_expr after type, type is %d %s\n", op, types[op]);
		tokenizer_next(ctx);
		//dprintf("double_expr call 2nd double_term\n");
		double_term(ctx, &t2);
		switch (op) {
			case TOKENIZER_PLUS:
				//dprintf("tokenizer plus\n");
				t1 = t1 + t2;
			break;
			case TOKENIZER_MINUS:
				//dprintf("tokenizer minus\n");
				t1 = t1 - t2;
			break;
			case TOKENIZER_AND:
				//dprintf("tokenizer and\n");
				t1 = (int64_t)t1 & (int64_t)t2;
			break;
			case TOKENIZER_OR:
				//dprintf("tokenizer or\n");
				t1 = (int64_t)t1 | (int64_t)t2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	//dprintf("double_expr done\n");
	*res = t1;
}

