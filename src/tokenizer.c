/*
 * BBC BASIC interpreter,
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
#define DEBUG_PRINTF(...)	kprintf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include <kernel.h>

#define MAX_NUMLEN 32

const char* types[] = {
	"ERROR",
	"END OF INPUT",
	"NUMBER",
	"HEX NUMBER",
	"STRING",
	"VARIABLE",
	"LET",
	"PRINT",
	"IF",
	"THEN",
	"ELSE",
	"CHAIN",
	"FOR",
	"TO",
	"STEP",
	"NEXT",
	"GOTOXY",
	"GOTO",
	"GOSUB",
	"RETURN",
	"CALL",
	"INPUT",
	"COLOUR",
	"COLOR",
	"EVAL",
	"OPENIN",
	"READ",
	"CLOSE",
	"EOF",
	"DEF",
	"PROC",
	"FN",
	"END",
	"REM",
	",",
	";",
	"+",
	"-",
	"AND",
	"OR",
	"*",
	"/",
	"MOD",
	")",
	"(",
	"<",
	">",
	"=",
	"NEW LINE",
	"&",
	"~",
	"GLOBAL",
	"SOCKREAD",
	"SOCKWRITE",
	"CONNECT",
	"SOCKCLOSE",
	"CLS",
	"GCOL",
	"LINE",
	"TRIANGLE",
	"RECTANGLE",
	"CIRCLE",
	"POINT",
	"OPENOUT",
	"OPENUP",
	"WRITE",
	"MKDIR",
	"RMDIR",
	"DELETE",
	"MOUNT",
};

struct keyword_token {
	char *keyword;
	int token;
};

static const struct keyword_token keywords[] = {
	{"LET", TOKENIZER_LET},
	{"PRINT", TOKENIZER_PRINT},
	{"IF", TOKENIZER_IF},
	{"THEN", TOKENIZER_THEN},
	{"ELSE", TOKENIZER_ELSE},
	{"DEF", TOKENIZER_DEF},
	{"FOR", TOKENIZER_FOR},
	{"TO", TOKENIZER_TO},
	{"STEP", TOKENIZER_STEP},
	{"NEXT", TOKENIZER_NEXT},
	{"GOTOXY", TOKENIZER_GOTOXY},
	{"GOTO", TOKENIZER_GOTO},
	{"GOSUB", TOKENIZER_GOSUB},
	{"RETURN", TOKENIZER_RETURN},
	{"CALL", TOKENIZER_CALL},
	{"INPUT", TOKENIZER_INPUT},
	{"COLOR", TOKENIZER_COLOR},
	{"COLOUR", TOKENIZER_COLOUR},
	{"END", TOKENIZER_END},
	{"CHAIN", TOKENIZER_CHAIN},
	{"EVAL", TOKENIZER_EVAL},
	{"OPENIN", TOKENIZER_OPENIN},
	{"READ", TOKENIZER_READ},
	{"CLOSE", TOKENIZER_CLOSE},
	{"EOF", TOKENIZER_EOF},
	{"REM", TOKENIZER_REM},
	{"GLOBAL", TOKENIZER_GLOBAL},
	{"SOCKREAD", TOKENIZER_SOCKREAD},
	{"SOCKWRITE", TOKENIZER_SOCKWRITE},
	{"CONNECT", TOKENIZER_CONNECT},
	{"SOCKCLOSE", TOKENIZER_SOCKCLOSE},
	{"CLS", TOKENIZER_CLS},
	{"GCOL", TOKENIZER_GCOL},
	{"LINE", TOKENIZER_LINE},
	{"TRIANGLE", TOKENIZER_TRIANGLE},
	{"RECTANGLE", TOKENIZER_RECTANGLE},
	{"CIRCLE", TOKENIZER_CIRCLE},
	{"POINT", TOKENIZER_POINT},
	{"OPENOUT", TOKENIZER_OPENOUT},
	{"OPENUP", TOKENIZER_OPENUP},
	{"WRITE", TOKENIZER_WRITE},
	{"MKDIR", TOKENIZER_MKDIR},
	{"RMDIR", TOKENIZER_RMDIR},
	{"DELETE", TOKENIZER_DELETE},
	{"MOUNT", TOKENIZER_MOUNT},
	{NULL, TOKENIZER_ERROR},
};

/*---------------------------------------------------------------------------*/
static int singlechar(struct ubasic_ctx* ctx)
{
	if(*ctx->ptr == '\n') {
		return TOKENIZER_CR;
	} else if(*ctx->ptr == ',') {
		return TOKENIZER_COMMA;
	} else if(*ctx->ptr == ';') {
		return TOKENIZER_SEMICOLON;
	} else if(*ctx->ptr == '+') {
		return TOKENIZER_PLUS;
	} else if(*ctx->ptr == '-') {
		return TOKENIZER_MINUS;
	} else if(*ctx->ptr == '&') {
		return TOKENIZER_AND;
	} else if(*ctx->ptr == '|') {
		return TOKENIZER_OR;
	} else if(*ctx->ptr == '*') {
		return TOKENIZER_ASTR;
	} else if(*ctx->ptr == '/') {
		return TOKENIZER_SLASH;
	} else if(*ctx->ptr == '%') {
		return TOKENIZER_MOD;
	} else if(*ctx->ptr == '(') {
		return TOKENIZER_LEFTPAREN;
	} else if(*ctx->ptr == ')') {
		return TOKENIZER_RIGHTPAREN;
	} else if(*ctx->ptr == '<') {
		return TOKENIZER_LT;
	} else if(*ctx->ptr == '>') {
		return TOKENIZER_GT;
	} else if(*ctx->ptr == '=') {
		return TOKENIZER_EQ;
	} else if(*ctx->ptr == '&') {
		return TOKENIZER_AMPERSAND;
	} else if (*ctx->ptr == '~') {
		return TOKENIZER_TILDE;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
int get_next_token(struct ubasic_ctx* ctx)
{
	struct keyword_token const *kt;
	int i;
	char hex = 0;

	DEBUG_PRINTF("get_next_token(): '%s'\n", ctx->ptr);

	if(*ctx->ptr == 0) {
		return TOKENIZER_ENDOFINPUT;
	}
	
	if (isdigit(*ctx->ptr) || *ctx->ptr == '&'|| *ctx->ptr == '.') {
		hex = (*ctx->ptr == '&') ? 1 : 0;
		if (hex) {
			ctx->ptr++;
			for (i = 0; i < MAX_NUMLEN; ++i) {
				if (!isxdigit(ctx->ptr[i])) {
					if (i > 1) {
						ctx->nextptr = ctx->ptr + i;
						return TOKENIZER_HEXNUMBER;
					} else {
						tokenizer_error_print(ctx, "Hexadecimal number too short");
						return TOKENIZER_ERROR;
					}
				}
				if (!isxdigit(ctx->ptr[i])) {
					tokenizer_error_print(ctx, "Malformed hexadecimal number");
					return TOKENIZER_ERROR;
				}
			}
		} else {
			/* Scan forwards up to MAX_NUMLEN characters */
			for (i = 0; i < MAX_NUMLEN; ++i) {
				/* Until we find a character that isnt part of a number */
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					if (i > 0) {
						ctx->nextptr = ctx->ptr + i;
						return TOKENIZER_NUMBER;
					} else {
						tokenizer_error_print(ctx, "Number too short");
						return TOKENIZER_ERROR;
					}
				}
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					tokenizer_error_print(ctx, "Malformed number");
					return TOKENIZER_ERROR;
				}
			}
		}
		tokenizer_error_print(ctx, "Number too long");
		return TOKENIZER_ERROR;
	} else if(singlechar(ctx)) {
		ctx->nextptr = ctx->ptr + 1;
		return singlechar(ctx);
	} else if(*ctx->ptr == '"') {
		ctx->nextptr = ctx->ptr;
		int strl = 0;
		do {
			++ctx->nextptr;
			if (++strl > 10240)
			{
				tokenizer_error_print(ctx, "String constant too long");
				break;
			}
		} while(*ctx->nextptr != '"');
		++ctx->nextptr;
		return TOKENIZER_STRING;
	} else {
		for(kt = keywords; kt->keyword != NULL; ++kt) {
			if(strncmp(ctx->ptr, kt->keyword, strlen(kt->keyword)) == 0) {
				ctx->nextptr = ctx->ptr + strlen(kt->keyword);
				return kt->token;
			}
		}
	}

	if ((*ctx->ptr >= 'a' && *ctx->ptr <= 'z') || (*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') || (*ctx->ptr == '$') || (*ctx->ptr == '#')) {
		ctx->nextptr = ctx->ptr;
		int varl = 0;
		while ((*ctx->nextptr >= 'a' && *ctx->nextptr <= 'z') || (*ctx->nextptr >= 'A' && *ctx->nextptr <= 'Z') || (*ctx->nextptr == '$') || (*ctx->nextptr == '#') || (*ctx->nextptr == '(')
				|| (varl > 0 && isdigit(*ctx->nextptr)))
		{
			//kprintf("%c", *ctx->nextptr);
			ctx->nextptr++;
			if (*ctx->nextptr == '(')
			{
			//kprintf("Nextptr found open bracket\n");
			int bracketdepth = 1;
			do
			{
				ctx->nextptr++;
				if (*ctx->nextptr == '(')
					bracketdepth++;
				else if (*ctx->nextptr == ')')
					bracketdepth--;
			}
			while (bracketdepth > 1 && *ctx->nextptr != 0);
			}
			else
			if (++varl > 60)
			{
				tokenizer_error_print(ctx, "Variable name too long");
				break;
			}
		}
		if (*ctx->nextptr == '$' || *ctx->nextptr == '#')
			ctx->nextptr++;
		//kprintf("Variable. nextptr = %08x ptr = %08x\n", ctx->nextptr, ctx->ptr);
		return TOKENIZER_VARIABLE;
	}

	
	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void
tokenizer_init(const char *program, struct ubasic_ctx* ctx)
{
	ctx->ptr = program;
	ctx->current_token = get_next_token(ctx);
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(struct ubasic_ctx* ctx)
{
	return ctx->current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(struct ubasic_ctx* ctx)
{

	if(tokenizer_finished(ctx)) {
		return;
	}

	DEBUG_PRINTF("tokenizer_next: %p\n", ctx->nextptr);
	ctx->ptr = ctx->nextptr;
	while(*ctx->ptr == ' ') {
		++ctx->ptr;
	}
	ctx->current_token = get_next_token(ctx);
	DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ctx->ptr, ctx->current_token);
	return;
}
/*---------------------------------------------------------------------------*/
int64_t tokenizer_num(struct ubasic_ctx* ctx, int token)
{
	return token == TOKENIZER_NUMBER ? atoll(ctx->ptr, 10) : atoll(ctx->ptr, 16);
}

void tokenizer_fnum(struct ubasic_ctx* ctx, int token, double* f)
{
	atof(ctx->ptr, f);
	return;
}
/*---------------------------------------------------------------------------*/
bool tokenizer_string(char *dest, int len, struct ubasic_ctx* ctx)
{
	char *string_end;
	int string_len;
	
	if(tokenizer_token(ctx) != TOKENIZER_STRING) {
		return true;
	}
	string_end = strchr(ctx->ptr + 1, '"');
	if(string_end == NULL) {
		tokenizer_error_print(ctx, "Unterminated \"");
		*dest = 0;
		return false;
	}
	string_len = string_end - ctx->ptr - 1;
	if(len < string_len) {
		string_len = len;
	}
	if (ctx->ptr == ctx->program_ptr) {
		tokenizer_error_print(ctx, "Unterminated \"");
		*dest = 0;
		return false;
	}
	memcpy(dest, ctx->ptr + 1, string_len);
	dest[string_len] = 0;
	return true;
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(struct ubasic_ctx* ctx, const char* error)
{
	ubasic_set_string_variable("ERROR$", error, ctx, false, false);
	ubasic_set_int_variable("ERROR", 1, ctx, false, false);
	ubasic_set_int_variable("ERRORLINE", ctx->current_linenum, ctx, false, false);
	if (ctx->eval_linenum == 0) {
		if (ctx->ended == 0) {
			setforeground(current_console, COLOUR_LIGHTRED);
			kprintf("Error on line %d: %s\n", ctx->current_linenum, error);
			setforeground(current_console, COLOUR_WHITE);
			ctx->ended = 1;
		}
	} else {
		if (!ctx->errored) {
			ctx->errored = true;
			setforeground(current_console, COLOUR_LIGHTRED);
			kprintf("%s\n", error);
			setforeground(current_console, COLOUR_WHITE);
			jump_linenum(ctx->eval_linenum, ctx);
		}
	}
}
/*---------------------------------------------------------------------------*/
int tokenizer_finished(struct ubasic_ctx* ctx)
{
	//kprintf("tokenizer_finished? %d %08x\n", *ctx->ptr, ctx->ptr);
	return *ctx->ptr == 0 || ctx->current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
const char* tokenizer_variable_name(struct ubasic_ctx* ctx)
{
	char varname[MAX_VARNAME];
	int count = 0;
	while (((*ctx->ptr >= 'a' && *ctx->ptr <= 'z') || (*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') || (*ctx->ptr == '$') || (*ctx->ptr == '#')) && count < MAX_VARNAME)
	{
		varname[count++] = *(ctx->ptr++);
	}
	varname[count] = 0;
	/* TODO: Validate variable name, weed out e.g. TEST$$ or $TEST$ which are not valid. */
	return gc_strdup(varname);
}
/*---------------------------------------------------------------------------*/

bool tokenizer_decimal_number(struct ubasic_ctx* ctx)
{
	const char* ptr = ctx->ptr;
	int whole_part_count = 0, decimal_part_count = 0;
	while (isdigit(*ptr)) {
		whole_part_count++;
		ptr++;
	}
	if (whole_part_count && *ptr == '.') {
		ptr++;
		while (isdigit(*ptr)) {
			ptr++;
			decimal_part_count++;
		}
		if (decimal_part_count) {
			return true;
		}
	}
	return false;
}