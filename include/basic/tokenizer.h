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
#include "kernel.h"

/**
 * @brief Maximum length for variable names.
 */
#define MAX_VARNAME 50

/**
 * @brief List of all tokens recognized by the interpreter.
 *
 * Note that built-in function names are not considered tokens. Instead, they are treated like user-defined
 * functions, but with hardcoded handlers rather than being redirected to user-defined code.
 *
 * The following #define generates an enum of token types, along with a corresponding array of string
 * representations for each token. These are used in basic.c to perform tokenization during program parsing.
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
    T(NO_TOKEN) 	/* 0 */ \
    T(ERROR) 		/* 1 */ \
    T(ENDOFINPUT) 	/* 2 */ \
    T(NUMBER) 		/* 3 */ \
    T(HEXNUMBER) 	/* 4 */ \
    T(STRING) 		/* 5 */ \
    T(VARIABLE) 	/* 6 */ \
    T(LET) 		/* 7 */ \
    T(PRINT) 		/* 8 */ \
    T(IF) 		/* 9 */ \
    T(THEN) 		/* 10 */ \
    T(ELSE) 		/* 11 */ \
    T(CHAIN) 		/* 12 */ \
    T(FOR) 		/* 13 */ \
    T(STEP) 		/* 14 */ \
    T(TO) 		/* 15 */ \
    T(NEXT) 		/* 16 */ \
    T(CURSOR) 		/* 17 */ \
    T(GOTO) 		/* 18 */ \
    T(GOSUB) 		/* 19 */ \
    T(RETURN) 		/* 20 */ \
    T(CALL) 		/* 21 */ \
    T(INPUT) 		/* 22 */ \
    T(COLOUR) 		/* 23 */ \
    T(COLOR) 		/* 24 */ \
    T(BACKGROUND) 	/* 25 */ \
    T(EVAL) 		/* 26 */ \
    T(CLOSE) 		/* 27 */ \
    T(DEF) 		/* 28 */ \
    T(PROC) 		/* 29 */ \
    T(ENDPROC) 		/* 30 */ \
    T(FN) 		/* 31 */ \
    T(END) 		/* 32 */ \
    T(REM) 		/* 33 */ \
    T(COMMA) 		/* 34 */ \
    T(SEMICOLON) 	/* 35 */ \
    T(PLUS) 		/* 36 */ \
    T(MINUS) 		/* 37 */ \
    T(AND) 		/* 38 */ \
    T(OR) 		/* 39 */ \
    T(NOT) 		/* 40 */ \
    T(EOR) 		/* 41 */ \
    T(ASTERISK) 	/* 42 */ \
    T(SLASH) 		/* 43 */ \
    T(MOD) 		/* 44 */ \
    T(OPENBRACKET) 	/* 45 */ \
    T(CLOSEBRACKET) 	/* 46 */ \
    T(LESSTHAN) 	/* 47 */ \
    T(GREATERTHAN) 	/* 48 */ \
    T(EQUALS) 		/* 49 */ \
    T(NEWLINE) 		/* 50 */ \
    T(AMPERSAND) 	/* 51 */ \
    T(TILDE) 		/* 52 */ \
    T(GLOBAL) 		/* 53 */ \
    T(SOCKREAD) 	/* 54 */ \
    T(SOCKWRITE) 	/* 55 */ \
    T(CONNECT) 		/* 56 */ \
    T(SOCKCLOSE) 	/* 57 */ \
    T(CLS) 		/* 58 */ \
    T(GCOL) 		/* 59 */ \
    T(LINE) 		/* 60 */ \
    T(TRIANGLE) 	/* 61 */ \
    T(RECTANGLE) 	/* 62 */ \
    T(CIRCLE) 		/* 63 */ \
    T(POINT) 		/* 64 */ \
    T(DATA) 		/* 65 */ \
    T(RESTORE) 		/* 66 */ \
    T(WRITE) 		/* 67 */ \
    T(MKDIR) 		/* 68 */ \
    T(RMDIR) 		/* 69 */ \
    T(DELETE) 		/* 70 */ \
    T(REPEAT) 		/* 71 */ \
    T(UNTIL) 		/* 72 */ \
    T(DIM) 		/* 73 */ \
    T(REDIM) 		/* 74 */ \
    T(PUSH) 		/* 75 */ \
    T(POKE) 		/* 76 */ \
    T(POKEW) 		/* 77 */ \
    T(POKED) 		/* 78 */ \
    T(POKEQ) 		/* 79 */ \
    T(POP) 		/* 80 */ \
    T(LOCAL) 		/* 81 */ \
    T(CHDIR) 		/* 82 */ \
    T(LIBRARY) 		/* 83 */ \
    T(YIELD) 		/* 84 */ \
    T(SETVARI) 		/* 85 */ \
    T(SETVARR) 		/* 86 */ \
    T(SETVARS) 		/* 87 */ \
    T(SPRITELOAD) 	/* 88 */ \
    T(SPRITEFREE) 	/* 89 */ \
    T(PLOT) 		/* 90 */ \
    T(AUTOFLIP) 	/* 91 */ \
    T(FLIP) 		/* 92 */ \
    T(KEYMAP) 		/* 93 */ \
    T(MOUNT) 		/* 94 */ \
    T(SETTIMEZONE) 	/* 95 */ \
    T(ENDIF) 		/* 96 */ \
    T(PLOTQUAD) 	/* 97 */ \
    T(ON) 		/* 98 */ \
    T(OFF) 		/* 99 */ \
    T(WHILE) 		/* 100 */ \
    T(ENDWHILE) 	/* 101 */ \
    T(SLEEP) 		/* 102 */ \
    T(CONTINUE) 	/* 103 */ \
    T(UDPBIND) 		/* 104 */ \
    T(UDPUNBIND) 	/* 105 */ \
    T(UDPWRITE) 	/* 106 */ \
    T(OUTPORT) 		/* 107 */ \
    T(OUTPORTW) 	/* 108 */ \
    T(OUTPORTD) 	/* 109 */ \
    T(KGET) 		/* 110 */ \

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
enum token_t tokenizer_token(struct basic_ctx* ctx);

/**
 * @brief Get integer number as next token
 * (do not advance the pointer)
 *
 * @param ctx context
 * @param token token (NUMBER or HEXNUMBER)
 * @return int64_t number read from program
 */
int64_t tokenizer_num(struct basic_ctx* ctx, enum token_t token);

/**
 * @brief Get real number as next token
 * (do not advance the pointer)
 *
 * @param ctx context
 * @param token token (NUMBER)
 * @param f number read from program
 */
void tokenizer_fnum(struct basic_ctx* ctx, enum token_t token, double* f);

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
bool tokenizer_finished(struct basic_ctx* ctx);

/**
 * @brief display an error to the terminal and end the program
 * @note If the program is running an EVAL, the error is printed
 * but the program is not ended, instead ERR$ and ERR are set.
 *
 * @param ctx context
 * @param error error message
 */
void tokenizer_error_print(struct basic_ctx* ctx, const char* error);

/**
 * @brief display an error to the terminal and end the program
 * @note If the program is running an EVAL, the error is printed
 * but the program is not ended, instead ERR$ and ERROR are set.
 *
 * @param ctx context
 * @param error error message
 */
void tokenizer_error_printf(struct basic_ctx* ctx, const char* fmt, ...) PRINTF_LIKE(2, 3);

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

