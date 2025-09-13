/**
 * @file basic/flow_control.c
 * @brief BASIC foow control functions (loops, conditionals, if/then/else etc)
 */
#include <kernel.h>
#include "basic/unified_expression.h"

extern bool debug;

bool conditional(struct basic_ctx* ctx) {
	basic_debug("line %ld conditional\n", ctx->current_linenum);

	return up_conditional(ctx);
}

void else_statement(struct basic_ctx* ctx)
{
	/* If we get to an ELSE, this means that we executed a THEN part of a block IF,
	 * so we must skip it and any content up until the next ENDIF 
	 */
	accept_or_return(ELSE, ctx);
	accept_or_return(NEWLINE, ctx);
	while (tokenizer_token(ctx) != ENDIF && !tokenizer_finished(ctx)) {
		tokenizer_next(ctx);
	}
	if (tokenizer_finished(ctx)) {
		tokenizer_error_print(ctx, "Block IF/THEN/ELSE without ENDIF");
	}
	accept_or_return(ENDIF, ctx);
	accept_or_return(NEWLINE, ctx);
}

void on_statement(struct basic_ctx* ctx)
{
	accept_or_return(ON, ctx);
	accept_or_return(ERROR, ctx);
	if (tokenizer_token(ctx) == OFF) {
		accept_or_return(OFF, ctx);
		buddy_free(ctx->allocator, ctx->error_handler);
		ctx->error_handler = NULL;
	} else if (tokenizer_token(ctx) == PROC) {
		char procname[MAX_STRINGLEN];
		accept_or_return(PROC, ctx);
		char* p = procname;
		size_t procnamelen = 0;
		while (*ctx->ptr != '\n' && *ctx->ptr != 0 && procnamelen < MAX_STRINGLEN - 1) {
			if (*ctx->ptr != ' ') {
				*(p++) = *(ctx->ptr++);
			}
			procnamelen++;
		}
		*p++ = 0;
		if (!basic_find_fn(procname, ctx)) {
			tokenizer_error_printf(ctx, "ON ERROR: No such procedure PROC%s", procname);
			return;
		}
		accept_or_return(VARIABLE, ctx);
		ctx->error_handler = buddy_strdup(ctx->allocator, procname);
	} else if (tokenizer_token(ctx) == GOTO || tokenizer_token(ctx) == GOSUB) {
		tokenizer_error_print(ctx, "ON ERROR GOTO and ON ERROR GOSUB are deprecated. Use ON ERROR PROC");
	} else {
		tokenizer_error_print(ctx, "Expected PROC or OFF");
	}
}

void off_statement(struct basic_ctx* ctx)
{
	accept_or_return(OFF, ctx);
	tokenizer_error_print(ctx, "OFF without ON ERROR");
}

void error_statement(struct basic_ctx* ctx)
{
	accept_or_return(ERROR, ctx);
	const char* message = str_expr(ctx);
	tokenizer_error_print(ctx, message);
}

void if_statement(struct basic_ctx* ctx)
{
	basic_debug("line %ld if_statement\n", ctx->current_linenum);

	accept_or_return(IF, ctx);
	bool r = conditional(ctx);
	accept_or_return(THEN, ctx);

	if (r) {
		basic_debug("conditional is true\n");
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF */
			accept_or_return(NEWLINE, ctx);
			return;
		}
		statement(ctx);
	} else {
		basic_debug("conditional is false\n");

		if (tokenizer_token(ctx) == NEWLINE) {
			/* --- multiline false-branch with nesting --- */
			/* Enter the block and scan forward once, respecting nested multiline IFs */
			accept_or_return(NEWLINE, ctx);

			int depth = 0;
			while (!tokenizer_finished(ctx)) {
				enum token_t t = tokenizer_token(ctx);

				/* Detect nested multiline IF: IF ... THEN NEWLINE */
				if (t == IF) {

					/* Advance until THEN or end-of-line/input */
					for (;;) {
						tokenizer_next(ctx);
						enum token_t tt = tokenizer_token(ctx);
						if (tt == THEN) {
							tokenizer_next(ctx);
							tt = tokenizer_token(ctx);
							/* Multiline only if THEN is immediately followed by NEWLINE */
							if (tt == NEWLINE) {
								depth++;
							}
							break;
						}
						if (tt == NEWLINE || tt == ENDOFINPUT) {
							break;
						}
					}
					/* Continue scanning from current position (no rewind) */
					continue;
				}

				/* At our depth, ELSE NEWLINE starts the else-block */
				if (t == ELSE) {
					tokenizer_next(ctx);
					if (depth == 0 && tokenizer_token(ctx) == NEWLINE) {
						accept_or_return(NEWLINE, ctx);
						return;
					}
					/* ELSE on the same line (single-line IF) or nested: ignore and continue */
					continue;
				}

				/* At our depth, ENDIF NEWLINE ends the whole IF without an else */
				if (t == ENDIF) {
					tokenizer_next(ctx);
					if (depth == 0) {
						accept_or_return(NEWLINE, ctx);
						return;
					}
					/* Close one nested multiline IF */
					depth--;
					continue;
				}

				/* Any other token: just keep scanning */
				tokenizer_next(ctx);
			}

			/* Reached end of input without matching ENDIF: treat as end of block */
			return;
		}

		/* Single-line IF ... THEN <stmt> [ELSE <stmt>] */
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
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr <= ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr >= ctx->for_stack[ctx->for_stack_ptr - 1].to));
		} else {
			int64_t incr = basic_get_numeric_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			basic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr <= ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr >= ctx->for_stack[ctx->for_stack_ptr - 1].to));
		}
		if (continue_loop) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		} else {
			buddy_free(ctx->allocator, ctx->for_stack[ctx->for_stack_ptr - 1].for_variable);
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
	size_t var_length;
	const char* for_variable = buddy_strdup(ctx->allocator, tokenizer_variable_name(ctx, &var_length));
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
	accept_or_return(ENDIF, ctx);
	accept_or_return(NEWLINE, ctx);
}

void end_statement(struct basic_ctx* ctx)
{
	accept_or_return(END, ctx);
	ctx->ended = true;
}

static bool seek_to_matching_token(struct basic_ctx* ctx, enum token_t open_tok, enum token_t close_tok, const char* name) {
	int depth = 1;
	int64_t line = 0;

	for (;;) {
		enum token_t tok = tokenizer_token(ctx);

		if (tok == ENDOFINPUT) {
			tokenizer_error_printf(ctx, "Unclosed %s", name);
			return false;
		}

		if (tok == NEWLINE) {
			tokenizer_next(ctx);
			tok = tokenizer_token(ctx);
			if (tok == NUMBER) {
				/* Store the last line number we saw */
				line = tokenizer_num(ctx, NUMBER);
			}
		}

		if (tok == open_tok) {
			depth++;
			tokenizer_next(ctx);
			continue;
		}

		if (tok == close_tok) {
			depth--;
			if (depth == 0) {
				/* Jump to the last line we saw, which is the one the closing token is on */
				jump_linenum(line, ctx);
				return true;
			}
		}

		tokenizer_next(ctx);
	}
}

void while_statement(struct basic_ctx* ctx)
{
	accept_or_return(WHILE, ctx);
	bool continue_loop = conditional(ctx);
	accept_or_return(NEWLINE, ctx);
	if (ctx->while_stack_ptr > 0 && ctx->while_stack[ctx->while_stack_ptr - 1] == (uint64_t)ctx->current_linenum) {
		/* We are already in this while loop */
	} else if (ctx->while_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		/* First time around this while loop */
		if (continue_loop) {
			ctx->while_stack[ctx->while_stack_ptr] = ctx->current_linenum;
			ctx->while_stack_ptr++;
		}
	} else {
		tokenizer_error_print(ctx, "WHILE stack exhausted");
		return;
	}
	if (!continue_loop) {
		/* conditional is false, jump to line AFTER the ENDWHILE */
		ctx->while_stack_ptr--;
		if (!seek_to_matching_token(ctx, WHILE, ENDWHILE, "WHILE")) {
			return;
		}
		while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
			tokenizer_next(ctx);
		}
	}
}

void endwhile_statement(struct basic_ctx* ctx)
{
	accept_or_return(ENDWHILE, ctx);
	accept_or_return(NEWLINE, ctx);

	if (ctx->while_stack_ptr > 0) {
		jump_linenum(ctx->while_stack[ctx->while_stack_ptr - 1], ctx);
	}
}

void continue_statement(struct basic_ctx* ctx)
{
	accept_or_return(CONTINUE, ctx);

	enum token_t kind = tokenizer_token(ctx);
	if (kind != WHILE && kind != FOR && kind != REPEAT) {
		tokenizer_error_print(ctx, "CONTINUE must be followed by WHILE, FOR, or REPEAT");
		return;
	}
	tokenizer_next(ctx);
	accept_or_return(NEWLINE, ctx);

	if (kind == WHILE) {
		if (ctx->while_stack_ptr == 0) {
			tokenizer_error_print(ctx, "CONTINUE WHILE used outside WHILE");
			return;
		}
		/* Your WHILE stack points at the WHILE line itself */
		jump_linenum(ctx->while_stack[ctx->while_stack_ptr - 1], ctx);
		return;
	}

	if (kind == FOR) {
		if (ctx->for_stack_ptr == 0) {
			tokenizer_error_print(ctx, "CONTINUE FOR used outside FOR");
			return;
		}
		if (!seek_to_matching_token(ctx, FOR, NEXT, "FOR")) {
			return;
		}
		/* We’re now positioned on NEXT; its handler will execute next. */
		return;
	}

	/* REPEAT */
	if (ctx->repeat_stack_ptr == 0) {
		tokenizer_error_print(ctx, "CONTINUE REPEAT used outside REPEAT");
		return;
	}
	if (!seek_to_matching_token(ctx, REPEAT, UNTIL, "REPEAT")) {
		return;
	}
	/* We’re now positioned on UNTIL; its handler will execute next. */
}
