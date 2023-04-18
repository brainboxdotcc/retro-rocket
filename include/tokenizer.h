/**
 * @file tokenizer.h
 * BBC BASIC interpreter,
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
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
 */
#pragma once

#include <kernel.h>

#define MAX_VARNAME 50

/**
 * These macros generate both an enum and an array of strings,
 * these are used as part of the tokenizer to parse the names of
 * the keywords out of the BASIC program without having to remember
 * to match up a string and its token enum value in two places.
 * It is also used for error reporting.
 */

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
#define GENERATE_ENUM_LIST(MACRO, NAME) \
	enum NAME { \
		MACRO(GENERATE_ENUM) \
	};
#define GENERATE_ENUM_STRING_NAMES(MACRO, NAME) \
	const char* NAME [] = { \
		MACRO(GENERATE_STRING) \
	};

#define TOKEN(T) \
	T(ERROR) \
	T(ENDOFINPUT) \
	T(NUMBER) \
	T(HEXNUMBER) \
	T(STRING) \
	T(VARIABLE) \
	T(LET) \
	T(PRINT) \
	T(IF) \
	T(THEN) \
	T(ELSE) \
	T(CHAIN) \
	T(FOR) \
	T(TO) \
	T(STEP) \
	T(NEXT) \
	T(CURSOR) \
	T(GOTO) \
	T(GOSUB) \
	T(RETURN) \
	T(CALL) \
	T(INPUT) \
	T(COLOUR) \
	T(COLOR) \
	T(BACKGROUND) \
	T(EVAL) \
	T(OPENIN) \
	T(READ) \
	T(CLOSE) \
	T(EOF) \
	T(DEF) \
	T(PROC) \
	T(FN) \
	T(END) \
	T(REM) \
	T(COMMA) \
	T(SEMICOLON) \
	T(PLUS) \
	T(MINUS) \
	T(AND) \
	T(OR) \
	T(ASTERISK) \
	T(SLASH) \
	T(MOD) \
	T(OPENBRACKET) \
	T(CLOSEBRACKET) \
	T(LESSTHAN) \
	T(GREATERTHAN) \
	T(EQUALS) \
	T(NEWLINE) \
	T(AMPERSAND) \
	T(TILDE) \
	T(GLOBAL) \
	T(SOCKREAD) \
	T(SOCKWRITE) \
	T(CONNECT) \
	T(SOCKCLOSE) \
	T(CLS) \
	T(GCOL) \
	T(LINE) \
	T(TRIANGLE) \
	T(RECTANGLE) \
	T(CIRCLE) \
	T(POINT) \
	T(OPENOUT) \
	T(OPENUP) \
	T(WRITE) \
	T(MKDIR) \
	T(RMDIR) \
	T(DELETE) \
	T(MOUNT)

GENERATE_ENUM_LIST(TOKEN, token_t)

void tokenizer_init(const char *program, struct ubasic_ctx* ctx);
void tokenizer_next(struct ubasic_ctx* ctx);
int tokenizer_token(struct ubasic_ctx* ctx);
int64_t tokenizer_num(struct ubasic_ctx* ctx, int token);
void tokenizer_fnum(struct ubasic_ctx* ctx, int token, double* f);
const char* tokenizer_variable_name(struct ubasic_ctx* ctx);
bool tokenizer_string(char *dest, int len, struct ubasic_ctx* ctx);
int tokenizer_finished(struct ubasic_ctx* ctx);
void tokenizer_error_print(struct ubasic_ctx* ctx, const char* error);
int get_next_token(struct ubasic_ctx* ctx);
bool tokenizer_decimal_number(struct ubasic_ctx* ctx);
