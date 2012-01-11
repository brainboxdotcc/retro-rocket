/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define DEBUG 0

#if DEBUG
#define DEBUG_PRINTF(...)  kprintf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include "../include/kernel.h"
#include "../include/ubasic.h"
#include "../include/tokenizer.h"
#include "../include/kprintf.h"
#include "../include/kmalloc.h"
#include "../include/string.h"
#include "../include/input.h"
#include "../include/video.h"
#include "../include/taskswitch.h"

static int expr(struct ubasic_ctx* ctx);
static void line_statement(struct ubasic_ctx* ctx);
static void statement(struct ubasic_ctx* ctx);
static const char* str_expr(struct ubasic_ctx* ctx);
const char* str_varfactor(struct ubasic_ctx* ctx);

/*---------------------------------------------------------------------------*/
struct ubasic_ctx* ubasic_init(const char *program, console* cons)
{
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	ctx->current_token = TOKENIZER_ERROR;	
	ctx->int_variables = NULL;
	ctx->str_variables = NULL;
	ctx->cons = cons;
	ctx->oldlen = 0;
	ctx->program_ptr = (char*)kmalloc(strlen(program) + 5000);
	strlcpy(ctx->program_ptr, program, strlen(program) + 5000);
	ctx->for_stack_ptr = ctx->gosub_stack_ptr = 0;
  	tokenizer_init(program, ctx);
	ctx->ended = 0;
	return ctx;
}

void ubasic_destroy(struct ubasic_ctx* ctx)
{
	for (; ctx->int_variables; ctx->int_variables = ctx->int_variables->next)
	{
		kfree(ctx->int_variables->varname);
		kfree(ctx->int_variables);
	}
	for (; ctx->str_variables; ctx->str_variables = ctx->str_variables->next)
	{
		kfree(ctx->str_variables->varname);
		kfree(ctx->str_variables->value);
		kfree(ctx->str_variables);
	}
	kfree((char*)ctx->program_ptr);
	kfree(ctx);
}

/*---------------------------------------------------------------------------*/
static void accept(int token, struct ubasic_ctx* ctx)
{
  if (token != tokenizer_token(ctx)) {
    DEBUG_PRINTF("Token not what was expected (expected %d, got %d)\n",
		 token, tokenizer_token(ctx));
    tokenizer_error_print(ctx, "Unexpected token");
  }
  DEBUG_PRINTF("Expected %d, got it\n", token);
  tokenizer_next(ctx);
  DEBUG_PRINTF("Tokenizer_next done\n");
}
/*---------------------------------------------------------------------------*/
static int varfactor(struct ubasic_ctx* ctx)
{
  int r;
  const char* vn = tokenizer_variable_name(ctx);
  //kprintf("varfactor '%s' ", vn);
  r = ubasic_get_int_variable(vn, ctx);
  //kprintf("varfactor val = %d\n", r);
  accept(TOKENIZER_VARIABLE, ctx);
  return r;
}

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	DEBUG_PRINTF("in str_varfactor\n");
	if (*ctx->ptr == '"')
	{
		DEBUG_PRINTF("*** QUOTED STRING LITERAL ***\n");
		tokenizer_string(ctx->string, sizeof(ctx->string), ctx);
		accept(TOKENIZER_STRING, ctx);
		return ctx->string;
	}
	else
	{
		const char* vn = tokenizer_variable_name(ctx);
		DEBUG_PRINTF("vn='%s'\n", vn);
		r = ubasic_get_string_variable(vn, ctx);
		accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

/*---------------------------------------------------------------------------*/
static int factor(struct ubasic_ctx* ctx)
{
  int r;

  DEBUG_PRINTF("factor: token %d\n", tokenizer_token(ctx));
  switch(tokenizer_token(ctx)) {
  case TOKENIZER_NUMBER:
    r = tokenizer_num(ctx);
    DEBUG_PRINTF("factor: number %d\n", r);
    accept(TOKENIZER_NUMBER, ctx);
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

static const char* str_factor(struct ubasic_ctx* ctx)
{
	const char* r;

	DEBUG_PRINTF("in str_factor\n");

	switch(tokenizer_token(ctx))
	{
		case TOKENIZER_LEFTPAREN:
			DEBUG_PRINTF("left paren\n");
			accept(TOKENIZER_LEFTPAREN, ctx);
			DEBUG_PRINTF("left paren 2\n");
			r = str_expr(ctx);
			DEBUG_PRINTF("left paren 3\n");
			accept(TOKENIZER_RIGHTPAREN, ctx);
			DEBUG_PRINTF("left paren 4\n");
		break;
		default:
			DEBUG_PRINTF("call str_varfactor\n");
			r = str_varfactor(ctx);
			DEBUG_PRINTF("done call varfactor: %s\n", r);
		break;
	}

	DEBUG_PRINTF("return %s\n", r);
	return r;
}

/*---------------------------------------------------------------------------*/
static int term(struct ubasic_ctx* ctx)
{
  int f1, f2;
  int op;

  f1 = factor(ctx);
  op = tokenizer_token(ctx);
  DEBUG_PRINTF("term: token %d\n", op);
  while(op == TOKENIZER_ASTR ||
	op == TOKENIZER_SLASH ||
	op == TOKENIZER_MOD) {
    tokenizer_next(ctx);
    f2 = factor(ctx);
    DEBUG_PRINTF("term: %d %d %d\n", f1, op, f2);
    switch(op) {
    case TOKENIZER_ASTR:
      f1 = f1 * f2;
      break;
    case TOKENIZER_SLASH:
      f1 = f1 / f2;
      break;
    case TOKENIZER_MOD:
      f1 = f1 % f2;
      break;
    }
    op = tokenizer_token(ctx);
  }
  DEBUG_PRINTF("term: %d\n", f1);
  return f1;
}
/*---------------------------------------------------------------------------*/
static int expr(struct ubasic_ctx* ctx)
{
  int t1, t2;
  int op;
  
  t1 = term(ctx);
  op = tokenizer_token(ctx);
  DEBUG_PRINTF("expr: token %d\n", op);
  while(op == TOKENIZER_PLUS ||
	op == TOKENIZER_MINUS ||
	op == TOKENIZER_AND ||
	op == TOKENIZER_OR) {
    tokenizer_next(ctx);
    t2 = term(ctx);
    DEBUG_PRINTF("expr: %d %d %d\n", t1, op, t2);
    switch(op) {
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
  DEBUG_PRINTF("expr: %d\n", t1);
  return t1;
}



static const char* str_expr(struct ubasic_ctx* ctx)
{
	char* t1;
	char* t2;
	char tmp[1024];
	int op;

	DEBUG_PRINTF("in str_expr\n");

	t1 = (char*)str_factor(ctx);
	DEBUG_PRINTF("one\n");
	op = tokenizer_token(ctx);

	DEBUG_PRINTF("op=%d\n", op);

	strlcpy(tmp, t1, 1024);
	
	while(op == TOKENIZER_PLUS)
	{
		DEBUG_PRINTF("plus\n");
		tokenizer_next(ctx);
		DEBUG_PRINTF("tok next\n");
		t2 = (char*)str_factor(ctx);
		DEBUG_PRINTF("got str_factor = %s\n", t2);
		switch (op)
		{
			case TOKENIZER_PLUS:
				DEBUG_PRINTF("doing plus\n");
				strlcat(tmp, t2, 1024);
				DEBUG_PRINTF("cat: %s\n",tmp);
			break;
		}
		op = tokenizer_token(ctx);
		DEBUG_PRINTF("new token\n");
	}
	return gc_strdup(tmp);
}
/*---------------------------------------------------------------------------*/
static int relation(struct ubasic_ctx* ctx)
{
  int r1, r2;
  int op;
  
  r1 = expr(ctx);
  op = tokenizer_token(ctx);
  DEBUG_PRINTF("relation: token %d\n", op);
  while(op == TOKENIZER_LT ||
	op == TOKENIZER_GT ||
	op == TOKENIZER_EQ) {
    tokenizer_next(ctx);
    r2 = expr(ctx);
    DEBUG_PRINTF("relation: %d %d %d\n", r1, op, r2);
    switch(op) {
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

/*---------------------------------------------------------------------------*/
void jump_linenum(int linenum, struct ubasic_ctx* ctx)
{
  tokenizer_init(ctx->program_ptr, ctx);
  while(tokenizer_num(ctx) != linenum) {
    do {
      do {
	tokenizer_next(ctx);
      } while(tokenizer_token(ctx) != TOKENIZER_CR &&
	      tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);
      if(tokenizer_token(ctx) == TOKENIZER_CR) {
	tokenizer_next(ctx);
      }
    } while(tokenizer_token(ctx) != TOKENIZER_NUMBER);
    DEBUG_PRINTF("jump_linenum: Found line %d\n", tokenizer_num(ctx));
  }
}
/*---------------------------------------------------------------------------*/
static void goto_statement(struct ubasic_ctx* ctx)
{
  accept(TOKENIZER_GOTO, ctx);
  jump_linenum(tokenizer_num(ctx), ctx);
}

static void colour_statement(struct ubasic_ctx* ctx, int tok)
{
	accept(tok, ctx);
	setforeground(ctx->cons, expr(ctx));
	accept(TOKENIZER_CR, ctx);
}

/*---------------------------------------------------------------------------*/
static void print_statement(struct ubasic_ctx* ctx)
{
  int numprints = 0;
  int no_newline = 0;
  accept(TOKENIZER_PRINT, ctx);
  do {
    no_newline = 0;
    DEBUG_PRINTF("Print loop\n");
    if(tokenizer_token(ctx) == TOKENIZER_STRING) {
      tokenizer_string(ctx->string, sizeof(ctx->string), ctx);
      kprintf("%s", ctx->string);
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_COMMA) {
      kprintf(" ");
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_SEMICOLON) {
      no_newline = 1;
      tokenizer_next(ctx);
    } else if (tokenizer_token(ctx) == TOKENIZER_PLUS) {
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_VARIABLE ||
	      tokenizer_token(ctx) == TOKENIZER_NUMBER) {
	    {
		    /* Check if it's a string or numeric expression */
		    const char* oldctx = ctx->ptr;
		    if (tokenizer_token(ctx) != TOKENIZER_NUMBER
				    && (*ctx->ptr == '"' || strchr(tokenizer_variable_name(ctx), '$')))
		    {
			    ctx->ptr = oldctx;
			    kprintf("%s", str_expr(ctx));
		    }
		    else
		    {
			    ctx->ptr = oldctx;
			    kprintf("%d", expr(ctx));
		    }
	    }
    } else {
      break;
    }
    numprints++;
  } while(tokenizer_token(ctx) != TOKENIZER_CR &&
	  tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT && numprints < 255);
  
  if (!no_newline)
	  kprintf("\n");

  DEBUG_PRINTF("End of print\n");
  tokenizer_next(ctx);
}
/*---------------------------------------------------------------------------*/
static void if_statement(struct ubasic_ctx* ctx)
{
  int r;
  
  accept(TOKENIZER_IF, ctx);

  r = relation(ctx);
  DEBUG_PRINTF("if_statement: relation %d\n", r);
  accept(TOKENIZER_THEN, ctx);
  if(r) {
    statement(ctx);
  } else {
    do {
      tokenizer_next(ctx);
    } while(tokenizer_token(ctx) != TOKENIZER_ELSE &&
	    tokenizer_token(ctx) != TOKENIZER_CR &&
	    tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);
    if(tokenizer_token(ctx) == TOKENIZER_ELSE) {
      tokenizer_next(ctx);
      statement(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_CR) {
      tokenizer_next(ctx);
    }
  }
}

static void chain_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_CHAIN, ctx);
	proc_load(str_expr(ctx), ctx->cons);
	accept(TOKENIZER_CR, ctx);
}

static void eval_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_EVAL, ctx);
	const char* v = str_expr(ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->oldlen == 0)
	{
		ubasic_set_string_variable("ERROR$", "", ctx);
		ubasic_set_int_variable("ERROR", 0, ctx);
		ctx->oldlen = strlen(ctx->program_ptr);
		strlcat(ctx->program_ptr, "9998 ", ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, v, ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, "\n9999 RETURN\n", ctx->oldlen + 5000);

		ctx->eval_linenum = ctx->current_linenum;

		ctx->gosub_stack[ctx->gosub_stack_ptr++] = ctx->current_linenum;

		jump_linenum(9998, ctx);
	}
	else
	{
		ctx->program_ptr[ctx->oldlen] = 0;
		ctx->oldlen = 0;
		ctx->eval_linenum = 0;
	}
}

static void input_statement(struct ubasic_ctx* ctx)
{
	const char* var;

	accept(TOKENIZER_INPUT, ctx);
	var = tokenizer_variable_name(ctx);
	DEBUG_PRINTF("varname: %s\n", var);
	accept(TOKENIZER_VARIABLE, ctx);

	DEBUG_PRINTF("Var Last Letter: %c\n", var[strlen(var) - 1]);

	if (kinput(10240, ctx->cons) != 0)
	{
		switch (var[strlen(var) - 1])
		{
			case '$':
				ubasic_set_string_variable(var, kgetinput(ctx->cons), ctx);
			break;
			default:
				ubasic_set_int_variable(var, atoi(kgetinput(ctx->cons)), ctx);
			break;
		}

		kfreeinput(ctx->cons);

		accept(TOKENIZER_CR, ctx);
	}
	else
	{
		jump_linenum(ctx->current_linenum, ctx);
	}
}

/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_ctx* ctx)
{
	const char* var;
	const char* _expr;

	var = tokenizer_variable_name(ctx);
	DEBUG_PRINTF("varname: %s\n", var);
	accept(TOKENIZER_VARIABLE, ctx);

	accept(TOKENIZER_EQ, ctx);

	DEBUG_PRINTF("Equals\n");

	DEBUG_PRINTF("Var Last Letter: %c\n", var[strlen(var) - 1]);

	switch (var[strlen(var) - 1])
	{
		case '$':
			DEBUG_PRINTF("Dollar eval: %s\n", var);
			_expr = str_expr(ctx);
			DEBUG_PRINTF("Returned str_expr: %s\n", _expr);
			ubasic_set_string_variable(var, _expr, ctx);
		break;
		default:
			DEBUG_PRINTF("Int eval: %s", var);
			ubasic_set_int_variable(var, expr(ctx), ctx);
		break;
	}
	DEBUG_PRINTF("let_statement: assign %s\n", var);
	accept(TOKENIZER_CR, ctx);
}
/*---------------------------------------------------------------------------*/
static void gosub_statement(struct ubasic_ctx* ctx)
{
  int linenum;
  accept(TOKENIZER_GOSUB, ctx);
  linenum = tokenizer_num(ctx);
  accept(TOKENIZER_NUMBER, ctx);
  accept(TOKENIZER_CR, ctx);
  if(ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
    ctx->gosub_stack[ctx->gosub_stack_ptr] = tokenizer_num(ctx);
    ctx->gosub_stack_ptr++;
    jump_linenum(linenum, ctx);
  } else {
    tokenizer_error_print(ctx, "gosub_statement: gosub stack exhausted\n");
  }
}
/*---------------------------------------------------------------------------*/
static void return_statement(struct ubasic_ctx* ctx)
{
  accept(TOKENIZER_RETURN, ctx);
  if(ctx->gosub_stack_ptr > 0) {
    ctx->gosub_stack_ptr--;
    jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr], ctx);
  } else {
    tokenizer_error_print(ctx, "return_statement: non-matching return\n");
  }
}
/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_ctx* ctx)
{
  const char* var;
  
  accept(TOKENIZER_NEXT, ctx);
  var = tokenizer_variable_name(ctx);
  accept(TOKENIZER_VARIABLE, ctx);
  if(ctx->for_stack_ptr > 0 &&
     !strcmp(var, ctx->for_stack[ctx->for_stack_ptr - 1].for_variable)) {
    ubasic_set_int_variable(var,
			ubasic_get_int_variable(var, ctx) + 1, ctx);
    if(ubasic_get_int_variable(var, ctx) <= ctx->for_stack[ctx->for_stack_ptr - 1].to) {
      jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
    } else {
      kfree(ctx->for_stack[ctx->for_stack_ptr].for_variable);
      ctx->for_stack_ptr--;
      accept(TOKENIZER_CR, ctx);
    }
  } else {
    tokenizer_error_print(ctx, "next_statement: non-matching next\n");
    accept(TOKENIZER_CR, ctx);
  }

}
/*---------------------------------------------------------------------------*/
static void for_statement(struct ubasic_ctx* ctx)
{
  const char* for_variable;
  int to;
  
  accept(TOKENIZER_FOR, ctx);
  for_variable = tokenizer_variable_name(ctx);
  accept(TOKENIZER_VARIABLE, ctx);
  accept(TOKENIZER_EQ, ctx);
  ubasic_set_int_variable(for_variable, expr(ctx), ctx);
  accept(TOKENIZER_TO, ctx);
  to = expr(ctx);
  accept(TOKENIZER_CR, ctx);

  if(ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
    ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx);
    ctx->for_stack[ctx->for_stack_ptr].for_variable = strdup(for_variable);
    ctx->for_stack[ctx->for_stack_ptr].to = to;
    DEBUG_PRINTF("for_statement: new for, var %s to %d\n",
		 ctx->for_stack[ctx->for_stack_ptr].for_variable,
		 ctx->for_stack[ctx->for_stack_ptr].to);
		 
    ctx->for_stack_ptr++;
  } else {
    tokenizer_error_print(ctx, "for_statement: for stack depth exceeded\n");
  }
}
/*---------------------------------------------------------------------------*/
static void end_statement(struct ubasic_ctx* ctx)
{
  accept(TOKENIZER_END, ctx);
  ctx->ended = 1;
}
/*---------------------------------------------------------------------------*/
static void statement(struct ubasic_ctx* ctx)
{
  int token;
  
  token = tokenizer_token(ctx);

  switch(token) {
  case TOKENIZER_COLOR:
    colour_statement(ctx, TOKENIZER_COLOR);
    break;
  case TOKENIZER_COLOUR:
    colour_statement(ctx, TOKENIZER_COLOUR);
    break;
  case TOKENIZER_CHAIN:
    chain_statement(ctx);
    break;
  case TOKENIZER_EVAL:
    eval_statement(ctx);
    break;
  case TOKENIZER_PRINT:
    print_statement(ctx);
    break;
  case TOKENIZER_IF:
    if_statement(ctx);
    break;
  case TOKENIZER_GOTO:
    goto_statement(ctx);
    break;
  case TOKENIZER_GOSUB:
    gosub_statement(ctx);
    break;
  case TOKENIZER_RETURN:
    return_statement(ctx);
    break;
  case TOKENIZER_FOR:
    for_statement(ctx);
    break;
  case TOKENIZER_NEXT:
    next_statement(ctx);
    break;
  case TOKENIZER_END:
    end_statement(ctx);
    break;
  case TOKENIZER_INPUT:
    input_statement(ctx);
    break;
  case TOKENIZER_LET:
    accept(TOKENIZER_LET, ctx);
    /* Fall through. */
  case TOKENIZER_VARIABLE:
    let_statement(ctx);
    break;
  default:
    tokenizer_error_print(ctx, "token not implemented\n");
  }
}
/*---------------------------------------------------------------------------*/
static void line_statement(struct ubasic_ctx* ctx)
{
  ctx->current_linenum = tokenizer_num(ctx);
  DEBUG_PRINTF("----------- Line number %d ---------\n", ctx->current_linenum);
  accept(TOKENIZER_NUMBER, ctx);
  //kprintf("%s\n", ctx->ptr);
  statement(ctx);
  return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(struct ubasic_ctx* ctx)
{
  if(ubasic_finished(ctx)) {
    //kprintf("End of program: '%s' %c\n", ctx->program_ptr, *(ctx->ptr - 2));
    DEBUG_PRINTF("uBASIC program finished\n");
    return;
  }

  line_statement(ctx);

  gc();
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(struct ubasic_ctx* ctx)
{
	int st = ctx->ended || tokenizer_finished(ctx);
	//kprintf("finish: st=%d ended=%d tokfin=%d\n", st, ctx->ended, tokenizer_finished(ctx));
	return st;
}
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(const char* var, const char* value, struct ubasic_ctx* ctx)
{
	/* first, itdentify the variable's type. Look for $ for string,
	 * and parenthesis for arrays
	 */
	const char last_letter = var[strlen(var) - 1];

	switch (last_letter)
	{
		case '$':
			ubasic_set_string_variable(var, value, ctx);
		break;
		case ')':
			ubasic_set_array_variable(var, atoi(value), ctx);
		break;
		default:
			ubasic_set_int_variable(var, atoi(value), ctx);
		break;
	}
}

int valid_string_var(const char* name)
{
	const char* i;
	//kprintf("check var '%s'\n", name);
	if (strlen(name) < 2)
		return 0;
	if (name[strlen(name) - 1] != '$')
		return 0;
	for (i = name; *i != '$'; i++)
	{
		if (*i == '$' && *(i + 1) != 0)
		       return 0;	
	}
	if ((*name >='A' && *name <= 'Z') || (*name >= 'a' && *name <= 'z'))
	{
		return 1;
	}
	else return 0;
}

int valid_int_var(const char* name)
{
	return 1;
}

void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx)
{
	if (!valid_string_var(var))
	{
		tokenizer_error_print(ctx, "Malformed variable name\n");
		return;
	}
	if (ctx->str_variables == NULL)
	{
		DEBUG_PRINTF("First string var\n");
		ctx->str_variables = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
		ctx->str_variables->next = NULL;
		ctx->str_variables->varname = strdup(var);
		ctx->str_variables->value = strdup(value);
		DEBUG_PRINTF("Done\n");
		return;
	}
	else
	{
		DEBUG_PRINTF("Not first string var\n");
		struct ub_var_string* cur = ctx->str_variables;
		for (; cur; cur = cur->next)
		{
			if (!strcmp(var, cur->varname))
			{
				kfree(cur->value);
				cur->value = strdup(value);
				DEBUG_PRINTF("Update value\n");
				return;
			}
		}
		DEBUG_PRINTF("Not first string var, create new\n");
		struct ub_var_string* newvar = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
		DEBUG_PRINTF("newvar=%08x\n\n", newvar);
		newvar->next = ctx->str_variables;
		newvar->varname = strdup(var);
		newvar->value = strdup(value);
		ctx->str_variables = newvar;
		DEBUG_PRINTF("Done\n");
	}
}

void ubasic_set_array_variable(const char* var, int value, struct ubasic_ctx* ctx)
{
}

void ubasic_set_int_variable(const char* var, int value, struct ubasic_ctx* ctx)
{
	if (!valid_int_var(var))
	{
		tokenizer_error_print(ctx, "Malformed variable name\n");
		return;
	}

	if (ctx->int_variables == NULL)
	{
		ctx->int_variables = (struct ub_var_int*)kmalloc(sizeof(struct ub_var_int));
		ctx->int_variables->next = NULL;
		ctx->int_variables->varname = strdup(var);
		ctx->int_variables->value = value;
		return;
	}
	else
	{
		struct ub_var_int* cur = ctx->int_variables;
		for (; cur; cur = cur->next)
		{
			if (!strcmp(var, cur->varname))
			{
				cur->value = value;
				return;
			}
		}
		struct ub_var_int* newvar = (struct ub_var_int*)kmalloc(sizeof(struct ub_var_int));
		newvar->next = ctx->int_variables;
		newvar->varname = strdup(var);
		newvar->value = value;
		ctx->int_variables = newvar;
	}
}
/*---------------------------------------------------------------------------*/
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx)
{
	DEBUG_PRINTF("ubasic_get_string_variable %s\n", var);

	if (ctx->str_variables == NULL)
	{
		DEBUG_PRINTF("no string vars\n");
		return ""; /* No such variable */
	}

	DEBUG_PRINTF("find string var%s\n", var);

	struct ub_var_string* cur = ctx->str_variables;
	for (; cur; cur = cur->next)
	{
		DEBUG_PRINTF("var search %s\n", var);
		if (!strcmp(var, cur->varname))
		{
			DEBUG_PRINTF("string var %s found val=%s\n", var, cur->value);
			return cur->value;
		}
	}

	tokenizer_error_print(ctx, "No such variable\n");
	return "";
}

int ubasic_get_int_variable(const char* var, struct ubasic_ctx* ctx)
{
	if (ctx->int_variables == NULL)
		return 0; /* No such variable */

	struct ub_var_int* cur = ctx->int_variables;
	for (; cur; cur = cur->next)
	{
		if (!strcmp(var, cur->varname))
		{
			return cur->value;
		}
	}

	tokenizer_error_print(ctx, "No such variable\n");
	return 0; /* No such variable */
}

/*---------------------------------------------------------------------------*/

