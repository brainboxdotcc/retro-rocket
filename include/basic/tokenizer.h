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
#define GENERATE_ENUM(NAME, ...) NAME,
#define GENERATE_STRING(NAME, ...) #NAME,
#define GENERATE_ENUM_LIST(MACRO, NAME) enum NAME { MACRO(GENERATE_ENUM) };
#define GENERATE_ENUM_STRING_NAMES(MACRO, NAME) const char* NAME [] = { MACRO(GENERATE_STRING) };

/* Markers for readability in the TOKEN list */
#define STMT     STMT
#define NONSTMT  NONSTMT

/* Conditional emit: only statement keywords produce a const in .kw.<name> */
#define EMIT_STMT(name) \
    static const int kw_##name __attribute__((section(".kw." #name), used)) = name;
#define EMIT_NONSTMT(name) /* nothing */

#define EMIT_FROM_FLAG(name, flag) EMIT_##flag(name)

/**
 * @brief All tokens recognised by the interpreter. Note that built in function names are NOT
 * tokens, they are parsed like user functions, just with a hard coded handler instead of
 * redirecting into the user program.
 *
 * The #define below builds an enum, and can also build an array of strings of the names in the
 * enum, which is built and used within basic.c for tokenization.
 */
#define TOKEN(T) \
    T(NO_TOKEN, NONSTMT) 	/* 0 */ \
    T(ERROR, STMT) 		/* 1 */ \
    T(ENDOFINPUT, NONSTMT) 	/* 2 */ \
    T(NUMBER, NONSTMT) 		/* 3 */ \
    T(HEXNUMBER, NONSTMT) 	/* 4 */ \
    T(STRING, NONSTMT) 		/* 5 */ \
    T(VARIABLE, NONSTMT) 	/* 6 */ \
    T(LET, STMT) 		/* 7 */ \
    T(PRINT, STMT) 		/* 8 */ \
    T(IF, STMT) 		/* 9 */ \
    T(THEN, STMT) 		/* 10 */ \
    T(ELSE, STMT) 		/* 11 */ \
    T(CHAIN, STMT) 		/* 12 */ \
    T(FOR, STMT) 		/* 13 */ \
    T(STEP, STMT) 		/* 14 */ \
    T(TO, STMT) 		/* 15 */ \
    T(NEXT, STMT) 		/* 16 */ \
    T(CURSOR, STMT) 		/* 17 */ \
    T(GOTO, STMT) 		/* 18 */ \
    T(GOSUB, STMT) 		/* 19 */ \
    T(RETURN, STMT) 		/* 20 */ \
    T(CALL, STMT) 		/* 21 */ \
    T(INPUT, STMT) 		/* 22 */ \
    T(COLOUR, STMT) 		/* 23 */ \
    T(COLOR, STMT) 		/* 24 */ \
    T(BACKGROUND, STMT) 	/* 25 */ \
    T(EVAL, STMT) 		/* 26 */ \
    T(CLOSE, STMT) 		/* 27 */ \
    T(DEF, STMT) 		/* 28 */ \
    T(PROC, STMT) 		/* 29 */ \
    T(ENDPROC, STMT) 		/* 30 */ \
    T(FN, NONSTMT) 		/* 31 */ \
    T(END, STMT) 		/* 32 */ \
    T(REM, STMT) 		/* 33 */ \
    T(COMMA, NONSTMT) 		/* 34 */ \
    T(SEMICOLON, NONSTMT) 	/* 35 */ \
    T(PLUS, NONSTMT) 		/* 36 */ \
    T(MINUS, NONSTMT) 		/* 37 */ \
    T(AND, STMT) 		/* 38 */ \
    T(OR, STMT) 		/* 39 */ \
    T(NOT, STMT) 		/* 40 */ \
    T(EOR, STMT) 		/* 41 */ \
    T(ASTERISK, NONSTMT) 	/* 42 */ \
    T(SLASH, NONSTMT) 		/* 43 */ \
    T(MOD, NONSTMT) 		/* 44 */ \
    T(OPENBRACKET, NONSTMT) 	/* 45 */ \
    T(CLOSEBRACKET, NONSTMT) 	/* 46 */ \
    T(LESSTHAN, NONSTMT) 	/* 47 */ \
    T(GREATERTHAN, NONSTMT) 	/* 48 */ \
    T(EQUALS, NONSTMT) 		/* 49 */ \
    T(NEWLINE, NONSTMT) 	/* 50 */ \
    T(AMPERSAND, NONSTMT) 	/* 51 */ \
    T(TILDE, NONSTMT) 		/* 52 */ \
    T(GLOBAL, STMT) 		/* 53 */ \
    T(SOCKREAD, STMT) 		/* 54 */ \
    T(SOCKWRITE, STMT) 		/* 55 */ \
    T(CONNECT, STMT) 		/* 56 */ \
    T(SOCKCLOSE, STMT) 		/* 57 */ \
    T(CLS, STMT) 		/* 58 */ \
    T(GCOL, STMT) 		/* 59 */ \
    T(LINE, STMT) 		/* 60 */ \
    T(TRIANGLE, STMT) 		/* 61 */ \
    T(RECTANGLE, STMT) 		/* 62 */ \
    T(CIRCLE, STMT) 		/* 63 */ \
    T(POINT, STMT) 		/* 64 */ \
    T(DATA, STMT) 		/* 65 */ \
    T(RESTORE, STMT) 		/* 66 */ \
    T(WRITE, STMT) 		/* 67 */ \
    T(MKDIR, STMT) 		/* 68 */ \
    T(RMDIR, STMT) 		/* 69 */ \
    T(DELETE, STMT) 		/* 70 */ \
    T(REPEAT, STMT) 		/* 71 */ \
    T(UNTIL, STMT) 		/* 72 */ \
    T(DIM, STMT) 		/* 73 */ \
    T(REDIM, STMT) 		/* 74 */ \
    T(PUSH, STMT) 		/* 75 */ \
    T(POKE, STMT) 		/* 76 */ \
    T(POKEW, STMT) 		/* 77 */ \
    T(POKED, STMT) 		/* 78 */ \
    T(POKEQ, STMT) 		/* 79 */ \
    T(POP, STMT) 		/* 80 */ \
    T(LOCAL, STMT) 		/* 81 */ \
    T(CHDIR, STMT) 		/* 82 */ \
    T(LIBRARY, STMT) 		/* 83 */ \
    T(YIELD, STMT) 		/* 84 */ \
    T(SETVARI, STMT) 		/* 85 */ \
    T(SETVARR, STMT) 		/* 86 */ \
    T(SETVARS, STMT) 		/* 87 */ \
    T(SPRITELOAD, STMT) 	/* 88 */ \
    T(SPRITEFREE, STMT) 	/* 89 */ \
    T(PLOT, STMT) 		/* 90 */ \
    T(AUTOFLIP, STMT) 		/* 91 */ \
    T(FLIP, STMT) 		/* 92 */ \
    T(KEYMAP, STMT) 		/* 93 */ \
    T(MOUNT, STMT) 		/* 94 */ \
    T(SETTIMEZONE, STMT) 	/* 95 */ \
    T(ENDIF, STMT) 		/* 96 */ \
    T(PLOTQUAD, STMT)	 	/* 97 */ \
    T(ON, STMT) 		/* 98 */ \
    T(OFF, STMT) 		/* 99 */ \
    T(WHILE, STMT) 		/* 100 */ \
    T(ENDWHILE, STMT) 		/* 101 */ \
    T(SLEEP, STMT) 		/* 102 */ \
    T(CONTINUE, STMT) 		/* 103 */ \
    T(UDPBIND, STMT) 		/* 104 */ \
    T(UDPUNBIND, STMT) 		/* 105 */ \
    T(UDPWRITE, STMT) 		/* 106 */ \
    T(OUTPORT, STMT) 		/* 107 */ \
    T(OUTPORTW, STMT) 		/* 108 */ \
    T(OUTPORTD, STMT) 		/* 109 */ \
    T(KGET, STMT) 		/* 110 */ \
    T(MODLOAD, STMT) 		/* 111 */ \
    T(MODUNLOAD, STMT) 		/* 112 */ \
    T(SOCKFLUSH, STMT) 		/* 113 */ \

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

