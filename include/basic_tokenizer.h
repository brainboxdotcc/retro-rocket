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
#define GENERATE_ENUM_LIST(MACRO, NAME) enum NAME { MACRO(GENERATE_ENUM) };
#define GENERATE_ENUM_STRING_NAMES(MACRO, NAME) const char* NAME [] = { MACRO(GENERATE_STRING) };

/**
 * @brief All tokens recognised by the interpreter. Note that built in function names are NOT
 * tokens, they are parsed like user functions, just with a hard coded handler instead of
 * redirecting into the user program.
 * 
 * The #define below builds an enum, and can also build an array of strings of the names in the
 * enum, which is built and used within basic.c for tokenization.
 */
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
	T(STEP) \
	T(TO) \
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
	T(RETPROC) \
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
	T(REPEAT) \
	T(UNTIL) \
	T(DIM) \
	T(REDIM) \
	T(PUSH) \
	T(POP) \
	T(LOCAL) \
	T(CHDIR) \
	T(LIBRARY) \
	T(YIELD) \
	T(MOUNT)

/*
 * Actually generate the enum, with the type token_t
 */
GENERATE_ENUM_LIST(TOKEN, token_t)

/**
 * @brief Initialise tokenizer
 * 
 * @param program program text
 * @param ctx context
 */
void tokenizer_init(const char *program, struct basic_ctx* ctx);

/**
 * @brief advance to next token
 * 
 * @param ctx context
 */
void tokenizer_next(struct basic_ctx* ctx);

/**
 * @brief peek to next token
 * 
 * @param ctx context
 * @return int token
 */
int tokenizer_token(struct basic_ctx* ctx);

/**
 * @brief Get integer number as next token
 * (do not advance the pointer)
 * 
 * @param ctx context
 * @param token token (NUMBER or HEXNUMBER)
 * @return int64_t number read from program
 */
int64_t tokenizer_num(struct basic_ctx* ctx, int token);

/**
 * @brief Get real number as next token
 * (do not advance the pointer)
 * 
 * @param ctx context
 * @param token token (NUMBER)
 * @param f number read from program
 */
void tokenizer_fnum(struct basic_ctx* ctx, int token, double* f);

/**
 * @brief Get a variable name as next token
 * (do not advance the pointer)
 * 
 * @param ctx context
 * @return const char* variable name
 */
const char* tokenizer_variable_name(struct basic_ctx* ctx);

/**
 * @brief Get a string constant as the next token
 * (do not advance the pointer)
 * 
 * @param dest destination string buffer
 * @param len length of destination buffer
 * @param ctx context
 * @return true if succesfully found a string constant
 */
bool tokenizer_string(char *dest, int len, struct basic_ctx* ctx);

/**
 * @brief Returns true if the program is finished
 * (does not advance the pointer)
 * 
 * @param ctx context
 * @return int true if the program has finished
 */
int tokenizer_finished(struct basic_ctx* ctx);

/**
 * @brief display an error to the terminal and end the program
 * @note If the program is running an EVAL, the error is printed
 * but the program is not ended, instead ERROR$ and ERROR are set.
 * 
 * @param ctx context
 * @param error error message
 */
void tokenizer_error_print(struct basic_ctx* ctx, const char* error);

/**
 * @brief Get the next token
 * (advances the pointer past the end of the token)
 * 
 * @param ctx context
 * @return int token found
 */
int get_next_token(struct basic_ctx* ctx);

/**
 * @brief Check if a decimal number is at the current
 * program pointer.
 * (does not advance the pointer)
 * 
 * @param ctx context
 * @return true if pointer points at a decimal number
 */
bool tokenizer_decimal_number(struct basic_ctx* ctx);
