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
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#define MAX_VARNAME 50

enum {
  TOKENIZER_ERROR,
  TOKENIZER_ENDOFINPUT,
  TOKENIZER_NUMBER,
  TOKENIZER_STRING,
  TOKENIZER_VARIABLE,
  TOKENIZER_LET,
  TOKENIZER_PRINT,
  TOKENIZER_IF,
  TOKENIZER_THEN,
  TOKENIZER_ELSE,
  TOKENIZER_CHAIN,
  TOKENIZER_FOR,
  TOKENIZER_TO,
  TOKENIZER_STEP,
  TOKENIZER_NEXT,
  TOKENIZER_GOTO,
  TOKENIZER_GOSUB,
  TOKENIZER_RETURN,
  TOKENIZER_CALL,
  TOKENIZER_INPUT,
  TOKENIZER_COLOUR,
  TOKENIZER_COLOR,
  TOKENIZER_EVAL,
  TOKENIZER_END,
  TOKENIZER_COMMA,
  TOKENIZER_SEMICOLON,
  TOKENIZER_PLUS,
  TOKENIZER_MINUS,
  TOKENIZER_AND,
  TOKENIZER_OR,
  TOKENIZER_ASTR,
  TOKENIZER_SLASH,
  TOKENIZER_MOD,
  TOKENIZER_LEFTPAREN,
  TOKENIZER_RIGHTPAREN,
  TOKENIZER_LT,
  TOKENIZER_GT,
  TOKENIZER_EQ,
  TOKENIZER_CR,
};

void tokenizer_init(const char *program, struct ubasic_ctx* ctx);
void tokenizer_next(struct ubasic_ctx* ctx);
int tokenizer_token(struct ubasic_ctx* ctx);
int tokenizer_num(struct ubasic_ctx* ctx);
const char* tokenizer_variable_name(struct ubasic_ctx* ctx);
void tokenizer_string(char *dest, int len, struct ubasic_ctx* ctx);
int tokenizer_finished(struct ubasic_ctx* ctx);
void tokenizer_error_print(struct ubasic_ctx* ctx, const char* error);
int get_next_token(struct ubasic_ctx* ctx);

#endif /* __TOKENIZER_H__ */
