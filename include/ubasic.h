/**
 * @file ubasic.h
 * @brief Retro Rocket BASIC interpreter
 * 
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * @note loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 * 
 * uBASIC is far more limited than the dialect implemented here. It only allowed
 * variables of one letter in length, and only integer variables, no PROC, FN,
 * or additional functions, no floating point or string ops, no INPUT,
 * just plain mathematical expressions, no ability to isolate execution into a
 * context, and was (and in parts still is) quite badly optimised. It was what
 * it was, a good starting off point.
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
#pragma once 

/**
 * @brief Maximum length of a string variable
 */
#define MAX_STRINGLEN 1024

/**
 * @brief Maximum stack depth of GOSUB, PROC, FN
 */
#define MAX_CALL_STACK_DEPTH 255

/**
 * @brief Maximum stack depth of FOR...NEXT
 */
#define MAX_LOOP_STACK_DEPTH 255

/**
 * @brief State for a FOR...NEXT loop
 */
typedef struct for_state {
        int64_t line_after_for;
        const char* for_variable;
        int64_t to;
        int64_t step;
} for_state;

/**
 * @brief An integer variable
 */
typedef struct ub_var_int {
	const char* varname;
	int64_t value;
	bool global;
	struct ub_var_int* next;
} ub_var_int;

/**
 * @brief A real (double) variable
 */
typedef struct ub_var_double {
	const char* varname;
	double value;
	bool global;
	struct ub_var_double* next;
} ub_var_double;

/**
 * @brief A string variable
 */
typedef struct ub_var_string {
	const char* varname;
	char* value;
	bool global;
	struct ub_var_string* next;
} ub_var_string;

/**
 * @brief Function or procedure
 */
typedef enum ub_fn_type {
	FT_FN,
	FT_PROC
} ub_fn_type;

/**
 * @brief Return type of FN, PROC
 */
typedef enum ub_return_type {
	RT_MAIN,	// In main
	RT_NONE,	// No return value
	RT_STRING,	// String return value
	RT_INT,		// Integer return value
	RT_FLOAT,	// Double return value
} ub_return_type;

/**
 * @brief FN/PROC parameter
 */
typedef struct ub_param {
	const char* name;
	struct ub_param* next;
} ub_param;

/**
 * @brief Procedure or function definition
 * Each has a name, a type, a starting line and a parameter list
 */
typedef struct ub_proc_fn_def {
	const char* name;
	ub_fn_type type;
	int64_t line;
	struct ub_param* params;
	struct ub_proc_fn_def* next;
} ub_proc_fn_def;

/**
 * @brief An array of integers
 */
typedef struct ub_var_int_array {
	uint64_t itemcount;
	const char* varname;
	int64_t* values;
	struct ub_var_int_array* next;
} ub_var_int_array;

/**
 * @brief An array of strings
 */
typedef struct ub_var_string_array {
	uint64_t itemcount;
	const char* varname;
	const char** values;
	struct ub_var_string_array* next;
} ub_var_string_array;

/**
 * @brief An array of real (double)
 */
typedef struct ub_var_double_array {
	uint64_t itemcount;
	const char* varname;
	double* values;
	struct ub_var_double_array* next;
} ub_var_double_array;

/**
 * @brief A generic array, we can use
 * this to represent any array regardless
 * of its contained type.
 */
typedef struct ub_var_generic_array {
	uint64_t itemcount;
	const char* varname;
	void* values_inaccesible;
	struct ub_var_generic_array* next;
} ub_var_generic_array;

/**
 * @brief Line reference in program.
 * Each program has a hashmap of these so it can find a line number
 * in O(1) time.
 */
typedef struct ub_line_ref {
	uint32_t line_number;
	const char* ptr;
} ub_line_ref;

/**
 * @brief Specil line number where EVAL code is inserted dynamically
 */
#define EVAL_LINE	999999998
/**
 * @brief RETURN statement after EVAL code
 * 
 */
#define EVAL_END_LINE	999999999

#define AUX(x) #x
#define STRINGIFY(x) AUX(x)

/**
 * @brief BASIC program context.
 * Every instance of a BASIC program has one of these,* also certain structures
 * such as functions will clone the context and run on the clone until the function
 * completes. Cloned contexts share variables and you should never call
 * ubasic_destroy() on them!
 */
typedef struct ubasic_ctx {
	/**
	 * @brief Pointer to the start of the next token
	 * Always between program_ptr and program_ptr + strlen(program_ptr)
	 */
        char const* ptr;
	/**
	 * @brief Pointer to the character after the next token.
	 * Always between program_ptr and program_ptr + strlen(program_ptr)
	 */
	char const* nextptr;
	/**
	 * @brief Numeric form of the token between ptr and nextptr.
	 * Should always be a value within the enum token_t
	 */
        int current_token;
	/**
	 * @brief Current line number
	 */
	int64_t current_linenum;
	/**
	 * @brief True if the program has thrown an error and should end.
	 * This may not actually cause termination of the program if we are
	 * inside an EVAL at the time.
	 */
	bool errored;
	/**
	 * @brief True if the program has ended, e.g. it reached an explicit
	 * END statement, or fell off the end of the program to the terminating
	 * null char.
	 */
        bool ended;
	/**
	 * @brief The whole program text, untokenized.
	 * May have been "cleaned" by an initial preprocessing phase which removes
	 * unneccesary spacing etc.
	 */
        char* program_ptr;
	/**
	 * @brief A context-local string buffer used for parsing function/procedure
	 * parameter lists.
	 */
        char string[MAX_STRINGLEN];
	/**
	 * @brief Local integer variable stack for variables declared within a
	 * function or procedure scope.
	 */
	struct ub_var_int* local_int_variables[MAX_CALL_STACK_DEPTH];
	/**
	 * @brief Local string variable stack for variables declared within a
	 * function or procedure scope.
	 */
	struct ub_var_string* local_string_variables[MAX_CALL_STACK_DEPTH];
	/**
	 * @brief Local real (double) variable stack for variables declared
	 * within a function or procedure scope.
	 */
	struct ub_var_double* local_double_variables[MAX_CALL_STACK_DEPTH];
	/**
	 * @brief Call stack, holds the return line numbers for each
	 * level of calls to a procedure, function or GOSUB
	 */
        uint64_t call_stack[MAX_CALL_STACK_DEPTH];
	/**
	 * @brief How far up the call stack we are. The call stack pointer
	 * starts at 0, and can go as high as MAX_CALL_STACK_DEPTH - 1
	 */
        uint64_t call_stack_ptr;
	/**
	 * @brief Repeat stack, holds the return line numbers for each
	 * level of REPEAT...UNTIL loop.
	 */
        uint64_t repeat_stack[MAX_LOOP_STACK_DEPTH];
	/**
	 * @brief How far up the REPEAT...UNTIL stack we are. The repeat
	 * stack pointer starts at 0, and can go as high as
	 * MAX_LOOP_STACK_DEPTH - 1
	 */
        uint64_t repeat_stack_ptr;
	/**
	 * @brief Previous program length, before an EVAL statement.
	 * An EVAL statement appends additional lines to the program
	 * beyond its original end, storing the previous size in this
	 * value. If this value is non-zero an EVAL is in progress,
	 * otherwise no EVAL is executing.
	 */
	size_t oldlen;
	/**
	 * @brief The return line number for an EVAL statement
	 */
	int64_t eval_linenum;
	/**
	 * @brief FOR stack, holds the return line numbers for each
	 * level of FOR...NEXT loop.
	 */
        struct for_state for_stack[MAX_LOOP_STACK_DEPTH];
	/**
	 * @brief How far up the FOR...NEXT stack we are. The FOR
	 * stack pointer starts at 0, and can go as high as
	 * MAX_LOOP_STACK_DEPTH - 1
	 */
        uint64_t for_stack_ptr;
	/**
	 * @brief Definitions for procedures and functions in the program
	 */
	struct ub_proc_fn_def* defs;
	/**
	 * @brief Global integer variable list
	 */
        struct ub_var_int* int_variables;
	/**
	 * @brief Global string variable list
	 */
	struct ub_var_string* str_variables;
	/**
	 * @brief Global double variable list
	 */
	struct ub_var_double* double_variables;
	/**
	 * @brief Global integer array variable list
	 */
	struct ub_var_int_array* int_array_variables;
	/**
	 * @brief Global string array variable list
	 */
	struct ub_var_string_array* string_array_variables;
	/**
	 * @brief Global double array variable list
	 */
	struct ub_var_double_array* double_array_variables;
	/**
	 * @brief I/O Console
	 */
	struct console* cons;
	/**
	 * @brief Function return type expected.
	 * This is only relavent when executing a function atomically.
	 */
	ub_return_type fn_type;
	/**
	 * @brief Function return value pointer.
	 * This is only relavent when executing a function atomically.
	 */
	void* fn_return;
	/**
	 * @brief Bracket depth when parsing function or procedure
	 * parameter lists.
	 */
	int bracket_depth;
	/**
	 * @brief Item start pointer when parsing function or procedure
	 * parameter lists.
	 */
	char const* item_begin;
	/**
	 * @brief Linked list of function parameters when parsing function
	 * or procedure parameter lists.
	 */
	struct ub_param* param;
	/**
	 * @brief Current graphics colour (GCOL) for graphics drawing commands
	 */
	int32_t graphics_colour;
	/**
	 * @brief Hashmap of lines for O(1) lookup of line numbers
	 */
	struct hashmap* lines;
	/**
	 * @brief Block IF...THEN...ELSE depth
	 */
	uint64_t if_nest_level;
	/**
	 * @brief highest line number in program
	 */
	int64_t highest_line;
} ubasic_ctx;

/**
 * @brief Integer function signature
 */
typedef int64_t (*builtin_int_fn)(struct ubasic_ctx* ctx);

/**
 * @brief String function signature
 */
typedef char* (*builtin_str_fn)(struct ubasic_ctx* ctx);

/**
 * @brief Real (double) function signature
 */
typedef void (*builtin_double_fn)(struct ubasic_ctx* ctx, double* res);

/**
 * @brief Builtin integer function
 */
typedef struct ubasic_int_fn
{
	builtin_int_fn handler;
	const char* name;
} ubasic_int_fn;

/**
 * @brief Builtin real (double) function
 */
typedef struct ubasic_double_fn
{
	builtin_double_fn handler;
	const char* name;
} ubasic_double_fn;

/**
 * @brief Builtin string function
 */
typedef struct ubasic_str_fn
{
	builtin_str_fn handler;
	const char* name;
} ubasic_str_fn;

typedef enum parameter_type_t {
	BIP_STRING,
	BIP_INT,
	BIP_DOUBLE,
	BIP_VARIABLE,
} parameter_type_t;

/**
 * @brief Begin parsing function parameters
 */
#define PARAMS_START \
	[[maybe_unused]] int itemtype = BIP_INT; \
	[[maybe_unused]] int64_t intval = 0; \
	[[maybe_unused]] double doubleval = 0; \
	[[maybe_unused]] char* strval = NULL; \
	[[maybe_unused]] char oldval = 0; \
	[[maybe_unused]] char oldct = 0; \
	[[maybe_unused]] char* oldptr = 0; \
	[[maybe_unused]] char const* oldnextptr = NULL; \
	[[maybe_unused]] int gotone = 0; \
	[[maybe_unused]] int bracket_depth = 0; \
	[[maybe_unused]] char const* item_begin = ctx->ptr;

/**
 * @brief Get a function parameter of type.
 * @param type Type of function parameter to get, fills one of the variables
 * strval, doubleval or intval.
 */
#define PARAMS_GET_ITEM(type) { gotone = 0; \
	while (!gotone) \
	{ \
		if (*ctx->ptr == '(') { \
			bracket_depth++; \
			if (bracket_depth == 1) \
				item_begin = ctx->ptr + 1; \
		} \
       		else if (*ctx->ptr == ')') \
			bracket_depth--; \
		if ((*ctx->ptr == ',' && bracket_depth == 1) || (*ctx->ptr == ')' && bracket_depth == 0)) { \
			gotone = 1; \
			oldval = *ctx->ptr; \
			oldct = ctx->current_token; \
			oldptr = (char*)ctx->ptr; \
			oldnextptr = ctx->nextptr; \
			ctx->nextptr = item_begin; \
			ctx->ptr = item_begin; \
			ctx->current_token = get_next_token(ctx); \
			*oldptr = 0; \
			itemtype = type; \
			if (itemtype == BIP_STRING) { \
				strval = (char*)str_expr(ctx); \
			} else if (itemtype == BIP_DOUBLE) { \
				double_expr(ctx, &doubleval); \
			} else if (itemtype == BIP_VARIABLE) { \
				strval = (char*)tokenizer_variable_name(ctx); \
			} else { \
				intval = expr(ctx); \
			} \
			*oldptr = oldval; \
			ctx->ptr = oldptr; \
			ctx->nextptr = oldnextptr; \
			ctx->current_token = oldct; \
			item_begin = ctx->ptr + 1; \
			gotone = 1; \
		} \
		if (bracket_depth == 0 || *ctx->ptr == 0) { \
			ctx->nextptr = ctx->ptr; \
			ctx->current_token = get_next_token(ctx); \
			gotone = 1; \
		} \
		ctx->ptr++; \
	} \
}

/**
 * @brief Ends fetching of function parameters, throwing an error if parameters still remain
 */
#define PARAMS_END(NAME) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
	} \
}

/*
 * Validation functions
 */
bool valid_int_var(const char* name);
bool valid_string_var(const char* name);
bool valid_double_var(const char* name);

/*
 * Builtin integer functions
 */
int64_t ubasic_abs(struct ubasic_ctx* ctx);
int64_t ubasic_len(struct ubasic_ctx* ctx);
int64_t ubasic_openin(struct ubasic_ctx* ctx);
int64_t ubasic_openout(struct ubasic_ctx* ctx);
int64_t ubasic_openup(struct ubasic_ctx* ctx);
int64_t ubasic_eof(struct ubasic_ctx* ctx);
int64_t ubasic_read(struct ubasic_ctx* ctx);
int64_t ubasic_instr(struct ubasic_ctx* ctx);
int64_t ubasic_asc(struct ubasic_ctx* ctx);
int64_t ubasic_getnamecount(struct ubasic_ctx* ctx);
int64_t ubasic_getsize(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_max_x(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_max_y(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_cur_x(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_cur_y(struct ubasic_ctx* ctx);
int64_t ubasic_getproccount(struct ubasic_ctx* ctx);
int64_t ubasic_getprocid(struct ubasic_ctx* ctx);
int64_t ubasic_getprocparent(struct ubasic_ctx* ctx);
int64_t ubasic_getproccpuid(struct ubasic_ctx* ctx);
int64_t ubasic_rgb(struct ubasic_ctx* ctx);
int64_t ubasic_get_free_mem(struct ubasic_ctx* ctx);
int64_t ubasic_get_used_mem(struct ubasic_ctx* ctx);
int64_t ubasic_get_total_mem(struct ubasic_ctx* ctx);
int64_t ubasic_sockstatus(struct ubasic_ctx* ctx);
int64_t ubasic_ctrlkey(struct ubasic_ctx* ctx);
int64_t ubasic_shiftkey(struct ubasic_ctx* ctx);
int64_t ubasic_altkey(struct ubasic_ctx* ctx);
int64_t ubasic_capslock(struct ubasic_ctx* ctx);
int64_t ubasic_random(struct ubasic_ctx* ctx);

/*
 * Builtin string functions
 */
char* ubasic_netinfo(struct ubasic_ctx* ctx);
char* ubasic_left(struct ubasic_ctx* ctx);
char* ubasic_right(struct ubasic_ctx* ctx);
char* ubasic_mid(struct ubasic_ctx* ctx);
char* ubasic_chr(struct ubasic_ctx* ctx);
char* ubasic_readstring(struct ubasic_ctx* ctx);
char* ubasic_getname(struct ubasic_ctx* ctx);
char* ubasic_getprocname(struct ubasic_ctx* ctx);
char* ubasic_dns(struct ubasic_ctx* ctx);
char* ubasic_ramdisk_from_device(struct ubasic_ctx* ctx);
char* ubasic_ramdisk_from_size(struct ubasic_ctx* ctx);
char* ubasic_inkey(struct ubasic_ctx* ctx);
char* ubasic_insocket(struct ubasic_ctx* ctx);
char* ubasic_upper(struct ubasic_ctx* ctx);
char* ubasic_lower(struct ubasic_ctx* ctx);
char* ubasic_tokenize(struct ubasic_ctx* ctx);
char* ubasic_csd(struct ubasic_ctx* ctx);

/*
 * File I/O functions
 */
void openin_statement(struct ubasic_ctx* ctx);
void openup_statement(struct ubasic_ctx* ctx);
void openout_statement(struct ubasic_ctx* ctx);
void read_statement(struct ubasic_ctx* ctx);
void close_statement(struct ubasic_ctx* ctx);
void eof_statement(struct ubasic_ctx* ctx);
void delete_statement(struct ubasic_ctx* ctx);
void mkdir_statement(struct ubasic_ctx* ctx);
void mount_statement(struct ubasic_ctx* ctx);
void rmdir_statement(struct ubasic_ctx* ctx);
void write_statement(struct ubasic_ctx* ctx);
void chdir_statement(struct ubasic_ctx* ctx);
char* ubasic_filetype(struct ubasic_ctx* ctx);

/*
 * Builtin real (double) functions
 */
void ubasic_sin(struct ubasic_ctx* ctx, double* res);
void ubasic_cos(struct ubasic_ctx* ctx, double* res);
void ubasic_tan(struct ubasic_ctx* ctx, double* res);
void ubasic_pow(struct ubasic_ctx* ctx, double* res);

/*
 * Context control functions
 */
struct ubasic_ctx* ubasic_init(const char *program, console* cons, uint32_t pid, const char* file, char** error);
void ubasic_destroy(struct ubasic_ctx* ctx);
void ubasic_run(struct ubasic_ctx* ctx);
bool ubasic_finished(struct ubasic_ctx* ctx);
bool jump_linenum(int64_t linenum, struct ubasic_ctx* ctx);
void line_statement(struct ubasic_ctx* ctx);
void statement(struct ubasic_ctx* ctx);
void accept(int token, struct ubasic_ctx* ctx);
void ubasic_parse_fn(struct ubasic_ctx* ctx);

/*
 * Variable getter/setter functions
 */
int64_t ubasic_get_int_variable(const char* varname, struct ubasic_ctx* ctx);
bool ubasic_get_double_variable(const char* var, struct ubasic_ctx* ctx, double* res);
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx);
void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, bool local, bool global);
void ubasic_set_double_variable(const char* var, const double value, struct ubasic_ctx* ctx, bool local, bool global);
void ubasic_set_int_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local, bool global);
ub_return_type ubasic_get_numeric_variable(const char* var, struct ubasic_ctx* ctx, double* res);
int64_t ubasic_get_numeric_int_variable(const char* var, struct ubasic_ctx* ctx);

/*
 * Array manipulation functions
 */
void ubasic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct ubasic_ctx* ctx);
void ubasic_set_string_array_variable(const char* var, int64_t index, const char* value, struct ubasic_ctx* ctx);
void ubasic_set_double_array_variable(const char* var, int64_t index, double value, struct ubasic_ctx* ctx);
bool ubasic_get_double_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx, double* ret);
int64_t ubasic_get_int_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx);
const char* ubasic_get_string_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx);
bool ubasic_dim_string_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool ubasic_dim_int_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool ubasic_dim_double_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool ubasic_redim_string_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool ubasic_redim_int_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool ubasic_redim_double_array(const char* var, int64_t size, struct ubasic_ctx* ctx);
bool varname_is_int_array_access(struct ubasic_ctx* ctx, const char* varname);
bool varname_is_string_array_access(struct ubasic_ctx* ctx, const char* varname);
bool varname_is_double_array_access(struct ubasic_ctx* ctx, const char* varname);
int64_t arr_variable_index(struct ubasic_ctx* ctx);
void ubasic_set_int_array(const char* var, int64_t value, struct ubasic_ctx* ctx);
void ubasic_set_string_array(const char* var, const char* value, struct ubasic_ctx* ctx);
void ubasic_set_double_array(const char* var, double value, struct ubasic_ctx* ctx);
void dim_statement(struct ubasic_ctx* ctx);
void redim_statement(struct ubasic_ctx* ctx);
void pop_statement(struct ubasic_ctx* ctx);
void push_statement(struct ubasic_ctx* ctx);
int64_t arr_expr_set_index(struct ubasic_ctx* ctx, const char* varname);
bool ubasic_pop_string_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx);
bool ubasic_pop_int_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx);
bool ubasic_pop_double_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx);
bool ubasic_push_string_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx);
bool ubasic_push_int_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx);
bool ubasic_push_double_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx);

/*
 * Integer expression evaluation
 */
int64_t expr(struct ubasic_ctx* ctx);
int64_t relation(struct ubasic_ctx* ctx);

/*
 * Real (double) expression evalutation
 */
void double_expr(struct ubasic_ctx* ctx, double* res);
void double_relation(struct ubasic_ctx* ctx, double* res);

/*
 * String expression evaluation
 */
const char* str_expr(struct ubasic_ctx* ctx);
int64_t str_relation(struct ubasic_ctx* ctx);

/*
 * Misc functions
 */
char* printable_syntax(struct ubasic_ctx* ctx);
void library_statement(struct ubasic_ctx* ctx);
void basic_free_defs(struct ubasic_ctx* ctx);