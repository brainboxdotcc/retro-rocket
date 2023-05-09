/**
 * @file basic/flow_control.c
 * @brief BASIC foow control functions (loops, conditionals, if/then/else etc)
 */
#include <kernel.h>

bool conditional(struct basic_ctx* ctx)
{
	char current_line[MAX_STRINGLEN];
	char* pos = strchr(ctx->ptr, '\n');
	char* end = strchr(ctx->ptr, 0);
	bool stringlike = false, real = false;
	strlcpy(current_line, ctx->ptr, pos ? pos - ctx->ptr + 1 : end - ctx->ptr + 1);
	for (char* n = current_line; *n && *n != '\n'; ++n) {
		if (strlen(n) >= 2 && isalnum(*n) && *(n + 1) == '$') {
			stringlike = true; /* String variable */
			break;
		} else if (strlen(n) >= 2 && isalnum(*n) && *(n + 1) == '#') {
			real = true; /* Real variable */
			break;
		} else if (strlen(n) >= 3 && isdigit(*n) && *(n + 1) == '.' && isdigit(*(n + 2))) {
			real = true; /* Decimal number */
			break;
		} else if (strlen(n) >= 5 && *n == ' ' && *(n + 1) == 'T' && *(n + 2) == 'H' && *(n + 3) == 'E' && *(n + 4) == 'N') {
			break;
		} else if (*n == '\n') {
			break;
		}
	}

	bool r = false;
	if (!real) {
		int64_t ri = stringlike ? str_relation(ctx) :  relation(ctx);
		r = (ri != 0);
	} else {
		double rd;
		double_relation(ctx, &rd);
		r = (rd != 0.0);
	}
	return r;
}

void else_statement(struct basic_ctx* ctx)
{
	/* If we get to an ELSE, this means that we executed a THEN part of a block IF,
	 * so we must skip it and any content up until the next ENDIF 
	 */
	accept_or_return(ELSE, ctx);
	accept_or_return(NEWLINE, ctx);
	while (tokenizer_token(ctx) != END && !tokenizer_finished(ctx)) {
		tokenizer_next(ctx);
		if (*ctx->ptr == 'I' && *(ctx->ptr+1) == 'F') {
			accept_or_return(IF, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
	}
	tokenizer_error_print(ctx, "Block IF/THEN/ELSE without ENDIF");
}

void if_statement(struct basic_ctx* ctx)
{
	accept_or_return(IF, ctx);
	bool r = conditional(ctx);
	accept_or_return(THEN, ctx);
	if (r) {
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF */
			accept_or_return(NEWLINE, ctx);
			ctx->if_nest_level++;
			return;
		}
		statement(ctx);
	} else {
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF, looking for ELSE */
			const char* old_start = ctx->ptr;
			const char* old_end = ctx->nextptr;
			while (!tokenizer_finished(ctx)) {
				int t = tokenizer_token(ctx);
				tokenizer_next(ctx);
				if (t == ELSE && tokenizer_token(ctx) == NEWLINE) {
					ctx->if_nest_level++;
					accept_or_return(NEWLINE, ctx);
					return;
				}
			}
			/* Didn't find a multiline ELSE, look for ENDIF */
			ctx->ptr = old_start;
			ctx->nextptr = old_end;
			ctx->current_token = 0;
			while (!tokenizer_finished(ctx)) {
				int t = tokenizer_token(ctx);
				tokenizer_next(ctx);
				if (t == END && tokenizer_token(ctx) == IF) {
					accept_or_return(IF, ctx);
					accept_or_return(NEWLINE, ctx);
					return;
				}
			}
		}
		do {
			tokenizer_next(ctx);
		} while (tokenizer_token(ctx) != ELSE && tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT);

		if (tokenizer_token(ctx) == ELSE) {
			tokenizer_next(ctx);
			statement(ctx);
		} else if (tokenizer_token(ctx) == NEWLINE) {
			tokenizer_next(ctx);
		}
	}
}

void gosub_statement(struct basic_ctx* ctx)
{
	int linenum;

	accept_or_return(GOSUB, ctx);
	linenum = tokenizer_num(ctx, NUMBER);
	accept_or_return(NUMBER, ctx);
	accept_or_return(NEWLINE, ctx);

	if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
		ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		jump_linenum(linenum, ctx);
	} else {
		tokenizer_error_print(ctx, "GOSUB: stack exhausted");
	}
}

void return_statement(struct basic_ctx* ctx)
{
	accept_or_return(RETURN, ctx);
	if (ctx->call_stack_ptr > 0) {
		free_local_heap(ctx);
		ctx->call_stack_ptr--;
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "RETURN without GOSUB");
	}
}

void next_statement(struct basic_ctx* ctx)
{
	accept_or_return(NEXT, ctx);
	if (ctx->for_stack_ptr > 0) {
		bool continue_loop = false;
		if (strchr(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, '#')) {
			double incr;
			basic_get_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx, &incr);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			basic_set_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		} else {
			int64_t incr = basic_get_numeric_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			basic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		}
		if (continue_loop) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		} else {
			kfree(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable);
			ctx->for_stack_ptr--;
			accept_or_return(NEWLINE, ctx);
		}

	} else {
		tokenizer_error_print(ctx, "NEXT without FOR");
		accept_or_return(NEWLINE, ctx);
	}
}

void for_statement(struct basic_ctx* ctx)
{
	accept_or_return(FOR, ctx);
	const char* for_variable = strdup(tokenizer_variable_name(ctx));
	accept_or_return(VARIABLE, ctx);
	accept_or_return(EQUALS, ctx);
	if (strchr(for_variable, '#')) {
		double d;
		double_expr(ctx, &d);
		basic_set_double_variable(for_variable, d, ctx, false, false);
	} else {
		basic_set_int_variable(for_variable, expr(ctx), ctx, false, false);
	}
	accept_or_return(TO, ctx);
	/* STEP needs special treatment, as it happens after an expression and is not separated by a comma */
	char* marker = NULL;
	for (char* n = (char*)ctx->ptr; *n && *n != '\n'; ++n) {
		if (strncmp(n, " STEP ", 6) == 0) {
			*n = '\n';
			marker = n;
			break;
		}
	}
	uint64_t to = expr(ctx), step = 1;
	tokenizer_next(ctx);
	if (marker) {
		*marker = ' ';
	}
	if (tokenizer_token(ctx) == STEP) {
		accept_or_return(STEP, ctx);
		step = expr(ctx);
		accept_or_return(NEWLINE, ctx);
	}

	if (ctx->for_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx, NUMBER);
		ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
		ctx->for_stack[ctx->for_stack_ptr].to = to;
		ctx->for_stack[ctx->for_stack_ptr].step = step;
		ctx->for_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "Too many FOR");
	}
}

void repeat_statement(struct basic_ctx* ctx)
{
	accept_or_return(REPEAT, ctx);
	accept_or_return(NEWLINE, ctx);
	if (ctx->repeat_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		ctx->repeat_stack[ctx->repeat_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->repeat_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "REPEAT stack exhausted");
	}

}


void until_statement(struct basic_ctx* ctx)
{
	accept_or_return(UNTIL, ctx);
	bool done = conditional(ctx);
	accept_or_return(NEWLINE, ctx);

	if (ctx->repeat_stack_ptr > 0) {
		if (!done) {
			jump_linenum(ctx->repeat_stack[ctx->repeat_stack_ptr - 1], ctx);
		} else {
			ctx->repeat_stack_ptr--;
		}
	} else {
		tokenizer_error_print(ctx, "UNTIL without REPEAT");
	}

}

void endif_statement(struct basic_ctx* ctx)
{
	if (ctx->if_nest_level == 0) {
		tokenizer_error_print(ctx, "ENDIF outside of block IF");
	}
	accept_or_return(IF, ctx);
	accept_or_return(NEWLINE, ctx);
	ctx->if_nest_level--;
}

void end_statement(struct basic_ctx* ctx)
{
	accept_or_return(END, ctx);
	if (tokenizer_token(ctx) == IF) {
		endif_statement(ctx);
		return;
	}
	ctx->ended = true;
}

