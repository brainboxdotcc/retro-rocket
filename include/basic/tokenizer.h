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
#define EMIT_STMT(name, dispatcher) \
    static const int kw_##name __attribute__((section(".kw." #name), used)) = name;
#define EMIT_NONSTMT(name, dispatcher) /* nothing */

#define EMIT_FROM_FLAG(name, flag, dispatcher) EMIT_##flag(name, dispatcher)

/* Per-token compile-time string length */
#define GENERATE_STRING_LENGTH(NAME, ...) (sizeof(#NAME) - 1u),

/* Builds: const unsigned char token_name_lengths[] = { ... }; */
#define GENERATE_ENUM_STRING_LENGTHS(MACRO, ARRAY_NAME) \
    const unsigned char ARRAY_NAME[] = { MACRO(GENERATE_STRING_LENGTH) };

/* Handler function type */
typedef void (*keyword_handler_t)(struct basic_ctx*);

/**
 * @brief All tokens recognised by the interpreter. Note that built in function names are NOT
 * tokens, they are parsed like user functions, just with a hard coded handler instead of
 * redirecting into the user program.
 *
 * The #define below builds an enum, and can also build an array of strings of the names in the
 * enum, which is built and used within basic.c for tokenization.
 */
#define TOKEN(T) \
    T(NO_TOKEN, NONSTMT, NULL) 				/* 0 */ \
    T(ERROR, STMT, error_statement) 			/* 1 */ \
    T(ENDOFINPUT, NONSTMT, NULL)		 	/* 2 */ \
    T(NUMBER, NONSTMT, NULL) 				/* 3 */ \
    T(HEXNUMBER, NONSTMT, NULL) 			/* 4 */ \
    T(STRING, NONSTMT, NULL) 				/* 5 */ \
    T(VARIABLE, NONSTMT, variable_statement)		/* 6 */ \
    T(LET, STMT, let_statement)				/* 7 */ \
    T(PRINT, STMT, print_statement) 			/* 8 */ \
    T(IF, STMT, if_statement) 				/* 9 */ \
    T(THEN, STMT, NULL) 				/* 10 */ \
    T(ELSE, STMT, else_statement) 			/* 11 */ \
    T(CHAIN, STMT, chain_statement) 			/* 12 */ \
    T(FOR, STMT, for_statement) 			/* 13 */ \
    T(STEP, STMT, NULL) 				/* 14 */ \
    T(TO, STMT, NULL) 					/* 15 */ \
    T(NEXT, STMT, next_statement) 			/* 16 */ \
    T(CURSOR, STMT, gotoxy_statement) 			/* 17 */ \
    T(GOTO, STMT, goto_statement) 			/* 18 */ \
    T(GOSUB, STMT, gosub_statement) 			/* 19 */ \
    T(RETURN, STMT, return_statement) 			/* 20 */ \
    T(CALL, STMT, NULL) 				/* 21 */ \
    T(INPUT, STMT, input_statement) 			/* 22 */ \
    T(COLOUR, STMT, colour_statement) 			/* 23 */ \
    T(COLOR, STMT, colour_statement) 			/* 24 */ \
    T(BACKGROUND, STMT, background_statement) 		/* 25 */ \
    T(EVAL, STMT, eval_statement) 			/* 26 */ \
    T(CLOSE, STMT, close_statement) 			/* 27 */ \
    T(DEF, STMT, def_statement) 			/* 28 */ \
    T(PROC, STMT, proc_statement) 			/* 29 */ \
    T(ENDPROC, STMT, endproc_statement) 		/* 30 */ \
    T(FN, NONSTMT, NULL) 				/* 31 */ \
    T(END, STMT, end_statement) 			/* 32 */ \
    T(REM, STMT, rem_statement) 			/* 33 */ \
    T(COMMA, NONSTMT, NULL) 				/* 34 */ \
    T(SEMICOLON, NONSTMT, NULL) 			/* 35 */ \
    T(PLUS, NONSTMT, NULL) 				/* 36 */ \
    T(MINUS, NONSTMT, NULL) 				/* 37 */ \
    T(AND, STMT, NULL) 					/* 38 */ \
    T(OR, STMT, NULL) 					/* 39 */ \
    T(NOT, STMT, NULL) 					/* 40 */ \
    T(EOR, STMT, NULL) 					/* 41 */ \
    T(ASTERISK, NONSTMT, NULL) 				/* 42 */ \
    T(SLASH, NONSTMT, NULL) 				/* 43 */ \
    T(MOD, NONSTMT, NULL) 				/* 44 */ \
    T(OPENBRACKET, NONSTMT, NULL) 			/* 45 */ \
    T(CLOSEBRACKET, NONSTMT, NULL) 			/* 46 */ \
    T(LESSTHAN, NONSTMT, NULL)	 			/* 47 */ \
    T(GREATERTHAN, NONSTMT, NULL) 			/* 48 */ \
    T(EQUALS, NONSTMT, eq_statement) 			/* 49 */ \
    T(NEWLINE, NONSTMT, newline_statement)		/* 50 */ \
    T(AMPERSAND, NONSTMT, NULL) 			/* 51 */ \
    T(TILDE, NONSTMT, NULL) 				/* 52 */ \
    T(GLOBAL, STMT, global_statement) 			/* 53 */ \
    T(SOCKREAD, STMT, sockread_statement) 		/* 54 */ \
    T(SOCKWRITE, STMT, sockwrite_statement) 		/* 55 */ \
    T(CONNECT, STMT, connect_statement) 		/* 56 */ \
    T(SOCKCLOSE, STMT, sockclose_statement) 		/* 57 */ \
    T(CLS, STMT, cls_statement) 			/* 58 */ \
    T(GCOL, STMT, gcol_statement) 			/* 59 */ \
    T(LINE, STMT, draw_line_statement) 			/* 60 */ \
    T(TRIANGLE, STMT, triangle_statement) 		/* 61 */ \
    T(RECTANGLE, STMT, rectangle_statement) 		/* 62 */ \
    T(CIRCLE, STMT, circle_statement) 			/* 63 */ \
    T(POINT, STMT, point_statement) 			/* 64 */ \
    T(DATA, STMT, data_statement) 			/* 65 */ \
    T(RESTORE, STMT, restore_statement) 		/* 66 */ \
    T(WRITE, STMT, write_statement) 			/* 67 */ \
    T(MKDIR, STMT, mkdir_statement) 			/* 68 */ \
    T(RMDIR, STMT, rmdir_statement) 			/* 69 */ \
    T(DELETE, STMT, delete_statement) 			/* 70 */ \
    T(REPEAT, STMT, repeat_statement) 			/* 71 */ \
    T(UNTIL, STMT, until_statement) 			/* 72 */ \
    T(DIM, STMT, dim_statement) 			/* 73 */ \
    T(REDIM, STMT, redim_statement) 			/* 74 */ \
    T(PUSH, STMT, push_statement) 			/* 75 */ \
    T(POKE, STMT, poke_statement) 			/* 76 */ \
    T(POKEW, STMT, pokew_statement) 			/* 77 */ \
    T(POKED, STMT, poked_statement) 			/* 78 */ \
    T(POKEQ, STMT, pokeq_statement) 			/* 79 */ \
    T(POP, STMT, pop_statement) 			/* 80 */ \
    T(LOCAL, STMT, local_statement) 			/* 81 */ \
    T(CHDIR, STMT, chdir_statement) 			/* 82 */ \
    T(LIBRARY, STMT, library_statement) 		/* 83 */ \
    T(YIELD, STMT, yield_statement) 			/* 84 */ \
    T(SETVARI, STMT, setvari_statement) 		/* 85 */ \
    T(SETVARR, STMT, setvarr_statement) 		/* 86 */ \
    T(SETVARS, STMT, setvars_statement) 		/* 87 */ \
    T(SPRITELOAD, STMT, loadsprite_statement) 		/* 88 */ \
    T(SPRITEFREE, STMT, freesprite_statement) 		/* 89 */ \
    T(PLOT, STMT, plot_statement) 			/* 90 */ \
    T(AUTOFLIP, STMT, autoflip_statement) 		/* 91 */ \
    T(FLIP, STMT, flip_statement) 			/* 92 */ \
    T(KEYMAP, STMT, keymap_statement) 			/* 93 */ \
    T(MOUNT, STMT, mount_statement) 			/* 94 */ \
    T(SETTIMEZONE, STMT, settimezone_statement) 	/* 95 */ \
    T(ENDIF, STMT, endif_statement) 			/* 96 */ \
    T(PLOTQUAD, STMT, plotquad_statement)	 	/* 97 */ \
    T(ON, STMT, on_statement) 				/* 98 */ \
    T(OFF, STMT, off_statement) 			/* 99 */ \
    T(WHILE, STMT, while_statement) 			/* 100 */ \
    T(ENDWHILE, STMT, endwhile_statement) 		/* 101 */ \
    T(SLEEP, STMT, sleep_statement) 			/* 102 */ \
    T(CONTINUE, STMT, continue_statement) 		/* 103 */ \
    T(UDPBIND, STMT, udpbind_statement) 		/* 104 */ \
    T(UDPUNBIND, STMT, udpunbind_statement) 		/* 105 */ \
    T(UDPWRITE, STMT, udpwrite_statement) 		/* 106 */ \
    T(OUTPORT, STMT, outport_statement) 		/* 107 */ \
    T(OUTPORTW, STMT, outportw_statement) 		/* 108 */ \
    T(OUTPORTD, STMT, outportd_statement) 		/* 109 */ \
    T(KGET, STMT, kget_statement) 			/* 110 */ \
    T(MODLOAD, STMT, modload_statement) 		/* 111 */ \
    T(MODUNLOAD, STMT, modunload_statement) 		/* 112 */ \
    T(SOCKFLUSH, STMT, sockflush_statement) 		/* 113 */ \
    T(MEMRELEASE, STMT, memrelease_statement) 		/* 114 */ \
    T(SOCKBINWRITE, STMT, sockbinwrite_statement) 	/* 115 */ \
    T(SOCKBINREAD, STMT, sockbinread_statement)		/* 116 */ \
    T(BINREAD, STMT, readbinary_statement) 		/* 117 */ \
    T(BINWRITE, STMT, writebinary_statement) 		/* 118 */ \
    T(GRAPHPRINT, STMT, graphprint_statement) 		/* 119 */ \
    T(VDU, STMT, vdu_statement) 			/* 120 */ \
    T(STREAM, STMT, stream_statement) 			/* 121 */ \
    T(CREATE, STMT, NULL) 				/* 122 */ \
    T(DESTROY, STMT, NULL) 				/* 123 */ \
    T(SOUND, STMT, sound_statement) 			/* 124 */ \
    T(VOLUME, STMT, NULL) 				/* 125 */ \
    T(PLAY, STMT, NULL) 				/* 126 */ \
    T(STOP, STMT, NULL) 				/* 127 */ \
    T(PAUSE, STMT, NULL) 				/* 128 */ \
    T(LOAD, STMT, NULL) 				/* 129 */ \
    T(UNLOAD, STMT, NULL) 				/* 130 */ \


GENERATE_ENUM_LIST(TOKEN, token_t)

/* Uniform handler type */
typedef void (*keyword_handler_t)(struct basic_ctx *);

/* Build a flat handler table indexed by token id */
#define GENERATE_HANDLER_ENTRY(NAME, FLAG, DISPATCHER) (keyword_handler_t)(DISPATCHER),

#define GENERATE_HANDLER_TABLE(ARRAY_NAME) \
    static const keyword_handler_t ARRAY_NAME[] = { \
        TOKEN(GENERATE_HANDLER_ENTRY) \
    };

/* Size helper */
#define DISPATCH_TABLE_COUNT(ARRAY_NAME) (sizeof(ARRAY_NAME) / sizeof((ARRAY_NAME)[0]))

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
 * @param count Upon return contains the string length of the variable name
 * @return const char* variable name
 */
const char* tokenizer_variable_name(struct basic_ctx* ctx, size_t* count);

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

