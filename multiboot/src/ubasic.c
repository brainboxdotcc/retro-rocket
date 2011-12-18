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

static int expr(struct ubasic_ctx* ctx);
static void line_statement(struct ubasic_ctx* ctx);
static void statement(struct ubasic_ctx* ctx);

/*---------------------------------------------------------------------------*/
struct ubasic_ctx* ubasic_init(const char *program)
{
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	ctx->current_token = TOKENIZER_ERROR;	
	ctx->program_ptr = program;
	ctx->for_stack_ptr = ctx->gosub_stack_ptr = 0;
  	tokenizer_init(program, ctx);
	ctx->ended = 0;
	return ctx;
}
/*---------------------------------------------------------------------------*/
static void accept(int token, struct ubasic_ctx* ctx)
{
  if (token != tokenizer_token(ctx)) {
    DEBUG_PRINTF("Token not what was expected (expected %d, got %d)\n",
		 token, tokenizer_token(ctx));
    tokenizer_error_print(ctx);
  }
  DEBUG_PRINTF("Expected %d, got it\n", token);
  tokenizer_next(ctx);
}
/*---------------------------------------------------------------------------*/
static int varfactor(struct ubasic_ctx* ctx)
{
  int r;
  DEBUG_PRINTF("varfactor: obtaining %d from variable %d\n", ctx->variables[tokenizer_variable_num(ctx)], tokenizer_variable_num(ctx));
  r = ubasic_get_variable(tokenizer_variable_num(ctx), ctx);
  accept(TOKENIZER_VARIABLE, ctx);
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
static void jump_linenum(int linenum, struct ubasic_ctx* ctx)
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
/*---------------------------------------------------------------------------*/
static void print_statement(struct ubasic_ctx* ctx)
{
  accept(TOKENIZER_PRINT, ctx);
  do {
    DEBUG_PRINTF("Print loop\n");
    if(tokenizer_token(ctx) == TOKENIZER_STRING) {
      tokenizer_string(ctx->string, sizeof(ctx->string), ctx);
      kprintf("%s", ctx->string);
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_COMMA) {
      kprintf(" ");
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_SEMICOLON) {
      tokenizer_next(ctx);
    } else if(tokenizer_token(ctx) == TOKENIZER_VARIABLE ||
	      tokenizer_token(ctx) == TOKENIZER_NUMBER) {
      kprintf("%d", expr(ctx));
    } else {
      break;
    }
  } while(tokenizer_token(ctx) != TOKENIZER_CR &&
	  tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);
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
/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_ctx* ctx)
{
  int var;

  var = tokenizer_variable_num(ctx);

  accept(TOKENIZER_VARIABLE, ctx);
  accept(TOKENIZER_EQ, ctx);
  ubasic_set_variable(var, expr(ctx), ctx);
  DEBUG_PRINTF("let_statement: assign %d to %d\n", ctx->variables[var], var);
  accept(TOKENIZER_CR, ctx);

}
/*---------------------------------------------------------------------------*/
static void
gosub_statement(struct ubasic_ctx* ctx)
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
    DEBUG_PRINTF("gosub_statement: gosub stack exhausted\n");
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
    DEBUG_PRINTF("return_statement: non-matching return\n");
  }
}
/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_ctx* ctx)
{
  int var;
  
  accept(TOKENIZER_NEXT, ctx);
  var = tokenizer_variable_num(ctx);
  accept(TOKENIZER_VARIABLE, ctx);
  if(ctx->for_stack_ptr > 0 &&
     var == ctx->for_stack[ctx->for_stack_ptr - 1].for_variable) {
    ubasic_set_variable(var,
			ubasic_get_variable(var, ctx) + 1, ctx);
    if(ubasic_get_variable(var, ctx) <= ctx->for_stack[ctx->for_stack_ptr - 1].to) {
      jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
    } else {
      ctx->for_stack_ptr--;
      accept(TOKENIZER_CR, ctx);
    }
  } else {
    DEBUG_PRINTF("next_statement: non-matching next (expected %d, found %d)\n", ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx->var);
    accept(TOKENIZER_CR, ctx);
  }

}
/*---------------------------------------------------------------------------*/
static void for_statement(struct ubasic_ctx* ctx)
{
  int for_variable, to;
  
  accept(TOKENIZER_FOR, ctx);
  for_variable = tokenizer_variable_num(ctx);
  accept(TOKENIZER_VARIABLE, ctx);
  accept(TOKENIZER_EQ, ctx);
  ubasic_set_variable(for_variable, expr(ctx), ctx);
  accept(TOKENIZER_TO, ctx);
  to = expr(ctx);
  accept(TOKENIZER_CR, ctx);

  if(ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
    ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx);
    ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
    ctx->for_stack[ctx->for_stack_ptr].to = to;
    DEBUG_PRINTF("for_statement: new for, var %d to %d\n",
		 ctx->for_stack[for_stack_ptr].for_variable,
		 ctx->for_stack[for_stack_ptr].to);
		 
    ctx->for_stack_ptr++;
  } else {
    DEBUG_PRINTF("for_statement: for stack depth exceeded\n");
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
  case TOKENIZER_LET:
    accept(TOKENIZER_LET, ctx);
    /* Fall through. */
  case TOKENIZER_VARIABLE:
    let_statement(ctx);
    break;
  default:
    DEBUG_PRINTF("ubasic.c: statement(): not implemented %d\n", token);
  }
}
/*---------------------------------------------------------------------------*/
static void line_statement(struct ubasic_ctx* ctx)
{
  DEBUG_PRINTF("----------- Line number %d ---------\n", tokenizer_num(ctx));
  /*    current_linenum = tokenizer_num();*/
  accept(TOKENIZER_NUMBER, ctx);
  statement(ctx);
  return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(struct ubasic_ctx* ctx)
{
  if(tokenizer_finished(ctx)) {
    DEBUG_PRINTF("uBASIC program finished\n");
    return;
  }

  line_statement(ctx);
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(struct ubasic_ctx* ctx)
{
  return ctx->ended || tokenizer_finished(ctx);
}
/*---------------------------------------------------------------------------*/
void
ubasic_set_variable(int varnum, int value, struct ubasic_ctx* ctx)
{
  if(varnum > 0 && varnum <= MAX_VARNUM) {
    ctx->variables[varnum] = value;
  }
}
/*---------------------------------------------------------------------------*/
int
ubasic_get_variable(int varnum, struct ubasic_ctx* ctx)
{
  if(varnum > 0 && varnum <= MAX_VARNUM) {
    return ctx->variables[varnum];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

