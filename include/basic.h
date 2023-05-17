/**
 * @file basic.h
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
 * @brief Special line number where EVAL code is inserted dynamically
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
 * @brief CPUID instruction result.
 */
typedef struct cpuid_result {
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;
} cpuid_result_t;

typedef struct g_cpuid_vendor {
	char const* varname;
	char const* vendor;
} g_cpuid_vendor_t;


#define DEBUG_BREAK	1
#define DEBUG_STEP	2
#define DEBUG_TRACE	4

/**
 * @brief BASIC program context.
 * Every instance of a BASIC program has one of these,* also certain structures
 * such as functions will clone the context and run on the clone until the function
 * completes. Cloned contexts share variables and you should never call
 * basic_destroy() on them!
 */
typedef struct basic_ctx {
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
	 * @brief Debug status, with values as of above, e.g. DEBUG_*
	 */
	uint8_t debug_status;
	/**
	 * @brief Active debug breakpoints (line numbers)
	 */
	uint64_t* debug_breakpoints;
	/**
	 * @brief Number of active breakpoints
	 */
	uint64_t debug_breakpoint_count;
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
	/*
	 * @brief Last CPUID instruction result
	 */
	cpuid_result_t last_cpuid_result;
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
} basic_ctx;

/**
 * @brief Integer function signature
 */
typedef int64_t (*builtin_int_fn)(struct basic_ctx* ctx);

/**
 * @brief String function signature
 */
typedef char* (*builtin_str_fn)(struct basic_ctx* ctx);

/**
 * @brief Real (double) function signature
 */
typedef void (*builtin_double_fn)(struct basic_ctx* ctx, double* res);

/**
 * @brief Builtin integer function
 */
typedef struct basic_int_fn
{
	builtin_int_fn handler;
	const char* name;
} basic_int_fn;

/**
 * @brief Builtin real (double) function
 */
typedef struct basic_double_fn
{
	builtin_double_fn handler;
	const char* name;
} basic_double_fn;

/**
 * @brief Builtin string function
 */
typedef struct basic_str_fn
{
	builtin_str_fn handler;
	const char* name;
} basic_str_fn;

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
		if (*ctx->ptr == '\n' || *ctx->ptr == 0) { \
			break; \
		} \
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
#define PARAMS_END(NAME, returnval) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return returnval; \
	} \
}

#define PARAMS_END_VOID(NAME) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return; \
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
int64_t basic_abs(struct basic_ctx* ctx);
int64_t basic_len(struct basic_ctx* ctx);
int64_t basic_openin(struct basic_ctx* ctx);
int64_t basic_openout(struct basic_ctx* ctx);
int64_t basic_openup(struct basic_ctx* ctx);
int64_t basic_eof(struct basic_ctx* ctx);
int64_t basic_read(struct basic_ctx* ctx);
int64_t basic_instr(struct basic_ctx* ctx);
int64_t basic_asc(struct basic_ctx* ctx);
int64_t basic_getnamecount(struct basic_ctx* ctx);
int64_t basic_getsize(struct basic_ctx* ctx);
int64_t basic_get_text_max_x(struct basic_ctx* ctx);
int64_t basic_get_text_max_y(struct basic_ctx* ctx);
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);
int64_t basic_getproccount(struct basic_ctx* ctx);
int64_t basic_getprocid(struct basic_ctx* ctx);
int64_t basic_getprocparent(struct basic_ctx* ctx);
int64_t basic_getproccpuid(struct basic_ctx* ctx);
int64_t basic_rgb(struct basic_ctx* ctx);
int64_t basic_get_free_mem(struct basic_ctx* ctx);
int64_t basic_get_used_mem(struct basic_ctx* ctx);
int64_t basic_get_total_mem(struct basic_ctx* ctx);
int64_t basic_sockstatus(struct basic_ctx* ctx);
int64_t basic_ctrlkey(struct basic_ctx* ctx);
int64_t basic_shiftkey(struct basic_ctx* ctx);
int64_t basic_altkey(struct basic_ctx* ctx);
int64_t basic_capslock(struct basic_ctx* ctx);
int64_t basic_random(struct basic_ctx* ctx);
int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx);
int64_t basic_legacy_cpuid(struct basic_ctx* ctx);
int64_t basic_cpuid(struct basic_ctx* ctx);
int64_t basic_atoi(struct basic_ctx* ctx);
int64_t basic_shl(struct basic_ctx* ctx);
int64_t basic_shr(struct basic_ctx* ctx);

/*
 * Builtin string functions
 */
char* basic_netinfo(struct basic_ctx* ctx);
char* basic_left(struct basic_ctx* ctx);
char* basic_right(struct basic_ctx* ctx);
char* basic_mid(struct basic_ctx* ctx);
char* basic_chr(struct basic_ctx* ctx);
char* basic_readstring(struct basic_ctx* ctx);
char* basic_getname(struct basic_ctx* ctx);
char* basic_getprocname(struct basic_ctx* ctx);
char* basic_dns(struct basic_ctx* ctx);
char* basic_ramdisk_from_device(struct basic_ctx* ctx);
char* basic_ramdisk_from_size(struct basic_ctx* ctx);
char* basic_inkey(struct basic_ctx* ctx);
char* basic_insocket(struct basic_ctx* ctx);
char* basic_upper(struct basic_ctx* ctx);
char* basic_lower(struct basic_ctx* ctx);
char* basic_tokenize(struct basic_ctx* ctx);
char* basic_csd(struct basic_ctx* ctx);
char* basic_cpugetbrand(struct basic_ctx* ctx);
char* basic_cpugetvendor(struct basic_ctx* ctx);
char* basic_intoasc(struct basic_ctx* ctx);
char* basic_ljust(struct basic_ctx* ctx);
char* basic_rjust(struct basic_ctx* ctx);
char* basic_ltrim(struct basic_ctx* ctx);
char* basic_rtrim(struct basic_ctx* ctx);
char* basic_trim(struct basic_ctx* ctx);
char* basic_itoa(struct basic_ctx* ctx);
char* basic_repeat(struct basic_ctx* ctx);
char* basic_reverse(struct basic_ctx* ctx);

/*
 * File I/O functions
 */
void openin_statement(struct basic_ctx* ctx);
void openup_statement(struct basic_ctx* ctx);
void openout_statement(struct basic_ctx* ctx);
void read_statement(struct basic_ctx* ctx);
void close_statement(struct basic_ctx* ctx);
void eof_statement(struct basic_ctx* ctx);
void delete_statement(struct basic_ctx* ctx);
void mkdir_statement(struct basic_ctx* ctx);
void mount_statement(struct basic_ctx* ctx);
void rmdir_statement(struct basic_ctx* ctx);
void write_statement(struct basic_ctx* ctx);
void chdir_statement(struct basic_ctx* ctx);
char* basic_filetype(struct basic_ctx* ctx);

/*
 * Builtin real (double) functions
 */
void basic_sin(struct basic_ctx* ctx, double* res);
void basic_cos(struct basic_ctx* ctx, double* res);
void basic_tan(struct basic_ctx* ctx, double* res);
void basic_pow(struct basic_ctx* ctx, double* res);
void basic_sqrt(struct basic_ctx* ctx, double* res);


/*
 * Context control functions
 */
struct basic_ctx* basic_init(const char *program, console* cons, uint32_t pid, const char* file, char** error);
void basic_destroy(struct basic_ctx* ctx);
void basic_run(struct basic_ctx* ctx);
bool basic_finished(struct basic_ctx* ctx);
bool jump_linenum(int64_t linenum, struct basic_ctx* ctx);
void line_statement(struct basic_ctx* ctx);
void statement(struct basic_ctx* ctx);
bool accept(int token, struct basic_ctx* ctx);
bool basic_parse_fn(struct basic_ctx* ctx);
struct basic_ctx* basic_clone(struct basic_ctx* old);
bool basic_finished(struct basic_ctx* ctx);

#define accept_or_return(token, ctx) \
	if (!accept(token, ctx)) { \
		return; \
	}

/*
 * Variable getter/setter functions
 */
int64_t basic_get_int_variable(const char* varname, struct basic_ctx* ctx);
bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res);
const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx);
void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool global);
void basic_set_double_variable(const char* var, const double value, struct basic_ctx* ctx, bool local, bool global);
void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool global);
ub_return_type basic_get_numeric_variable(const char* var, struct basic_ctx* ctx, double* res);
int64_t basic_get_numeric_int_variable(const char* var, struct basic_ctx* ctx);

/*
 * Array manipulation functions
 */
void basic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct basic_ctx* ctx);
void basic_set_string_array_variable(const char* var, int64_t index, const char* value, struct basic_ctx* ctx);
void basic_set_double_array_variable(const char* var, int64_t index, double value, struct basic_ctx* ctx);
bool basic_get_double_array_variable(const char* var, int64_t index, struct basic_ctx* ctx, double* ret);
int64_t basic_get_int_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);
const char* basic_get_string_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);
bool basic_dim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_dim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_dim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool varname_is_int_array_access(struct basic_ctx* ctx, const char* varname);
bool varname_is_string_array_access(struct basic_ctx* ctx, const char* varname);
bool varname_is_double_array_access(struct basic_ctx* ctx, const char* varname);
int64_t arr_variable_index(struct basic_ctx* ctx);
void basic_set_int_array(const char* var, int64_t value, struct basic_ctx* ctx);
void basic_set_string_array(const char* var, const char* value, struct basic_ctx* ctx);
void basic_set_double_array(const char* var, double value, struct basic_ctx* ctx);
void dim_statement(struct basic_ctx* ctx);
void redim_statement(struct basic_ctx* ctx);
void pop_statement(struct basic_ctx* ctx);
void push_statement(struct basic_ctx* ctx);
int64_t arr_expr_set_index(struct basic_ctx* ctx, const char* varname);
bool basic_pop_string_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_pop_int_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_pop_double_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_push_string_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);
bool basic_push_int_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);
bool basic_push_double_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);

/*
 * Integer expression evaluation
 */
int64_t expr(struct basic_ctx* ctx);
int64_t relation(struct basic_ctx* ctx);

/*
 * Real (double) expression evalutation
 */
void double_expr(struct basic_ctx* ctx, double* res);
void double_relation(struct basic_ctx* ctx, double* res);

/*
 * String expression evaluation
 */
const char* str_expr(struct basic_ctx* ctx);
int64_t str_relation(struct basic_ctx* ctx);

/*
 * Misc functions
 */
char* printable_syntax(struct basic_ctx* ctx);
void library_statement(struct basic_ctx* ctx);
void basic_free_defs(struct basic_ctx* ctx);
void begin_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx);
uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx);
bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx);
bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx);
bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx);
int64_t basic_val(struct basic_ctx* ctx);
int64_t basic_hexval(struct basic_ctx* ctx);
int64_t basic_octval(struct basic_ctx* ctx);
void basic_realval(struct basic_ctx* ctx, double* res);
char* basic_str(struct basic_ctx* ctx);
char* basic_bool(struct basic_ctx* ctx);
void set_system_variables(struct basic_ctx* ctx, uint32_t pid);
void let_statement(struct basic_ctx* ctx, bool global, bool local);
char basic_builtin_str_fn(const char* fn_name, struct basic_ctx* ctx, char** res);
int64_t basic_eval_int_fn(const char* fn_name, struct basic_ctx* ctx);
void basic_eval_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res);
const char* basic_eval_str_fn(const char* fn_name, struct basic_ctx* ctx);
char basic_builtin_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res);
char basic_builtin_int_fn(const char* fn_name, struct basic_ctx* ctx, int64_t* res);
struct ub_proc_fn_def* basic_find_fn(const char* name, struct basic_ctx* ctx);
void init_local_heap(struct basic_ctx* ctx);
void free_local_heap(struct basic_ctx* ctx);
bool is_builtin_double_fn(const char* fn_name);
void def_statement(struct basic_ctx* ctx);
void proc_statement(struct basic_ctx* ctx);
void eq_statement(struct basic_ctx* ctx);
void retproc_statement(struct basic_ctx* ctx);

/* Sockets functionality */
void sockwrite_statement(struct basic_ctx* ctx);
char* basic_dns(struct basic_ctx* ctx);
char* basic_netinfo(struct basic_ctx* ctx);
int64_t basic_sockstatus(struct basic_ctx* ctx);
char* basic_insocket(struct basic_ctx* ctx);
void sockclose_statement(struct basic_ctx* ctx);
void connect_statement(struct basic_ctx* ctx);
void sockread_statement(struct basic_ctx* ctx);

/* Low level statements */
void write_cpuid(struct basic_ctx* ctx, int leaf);
void write_cpuidex(struct basic_ctx* ctx, int leaf, int subleaf);
int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg);
int64_t basic_legacy_cpuid(struct basic_ctx* ctx);
int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx);
char* basic_cpugetbrand(struct basic_ctx* ctx);
char* basic_cpugetvendor(struct basic_ctx* ctx);
char* basic_intoasc(struct basic_ctx* ctx);
int64_t basic_cpuid(struct basic_ctx* ctx);

/* Graphics statements */
int64_t basic_rgb(struct basic_ctx* ctx);
void circle_statement(struct basic_ctx* ctx);
void triangle_statement(struct basic_ctx* ctx);
void point_statement(struct basic_ctx* ctx);
void draw_line_statement(struct basic_ctx* ctx);
void gcol_statement(struct basic_ctx* ctx);
void rectangle_statement(struct basic_ctx* ctx);

/* Console functions */
int64_t basic_get_text_max_x(struct basic_ctx* ctx);
int64_t basic_get_text_max_y(struct basic_ctx* ctx);
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);
char* basic_inkey(struct basic_ctx* ctx);
int64_t basic_ctrlkey(struct basic_ctx* ctx);
int64_t basic_shiftkey(struct basic_ctx* ctx);
int64_t basic_altkey(struct basic_ctx* ctx);
int64_t basic_capslock(struct basic_ctx* ctx);
void input_statement(struct basic_ctx* ctx);
void cls_statement(struct basic_ctx* ctx);
void gotoxy_statement(struct basic_ctx* ctx);
void print_statement(struct basic_ctx* ctx);
void colour_statement(struct basic_ctx* ctx, int tok);
void background_statement(struct basic_ctx* ctx);

/* Flow control */
bool conditional(struct basic_ctx* ctx);
void else_statement(struct basic_ctx* ctx);
void if_statement(struct basic_ctx* ctx);
void gosub_statement(struct basic_ctx* ctx);
void return_statement(struct basic_ctx* ctx);
void next_statement(struct basic_ctx* ctx);
void for_statement(struct basic_ctx* ctx);
void repeat_statement(struct basic_ctx* ctx);
void until_statement(struct basic_ctx* ctx);
void endif_statement(struct basic_ctx* ctx);
void end_statement(struct basic_ctx* ctx);
void panic_statement(struct basic_ctx* ctx);

/* Process/memory functions */
int64_t basic_getproccount(struct basic_ctx* ctx);
int64_t basic_get_free_mem(struct basic_ctx* ctx);
int64_t basic_get_used_mem(struct basic_ctx* ctx);
int64_t basic_get_total_mem(struct basic_ctx* ctx);
int64_t basic_getprocid(struct basic_ctx* ctx);
char* basic_getprocname(struct basic_ctx* ctx);
int64_t basic_getprocparent(struct basic_ctx* ctx);
int64_t basic_getproccpuid(struct basic_ctx* ctx);

/* Reflection stuff */
int64_t basic_getvar_int(struct basic_ctx* ctx);
void basic_getvar_real(struct basic_ctx* ctx, double* res);
char* basic_getvar_string(struct basic_ctx* ctx);

int64_t basic_existsvar_int(struct basic_ctx* ctx);
int64_t basic_existsvar_real(struct basic_ctx* ctx);
int64_t basic_existsvar_string(struct basic_ctx* ctx);

void setvari_statement(struct basic_ctx* ctx);
void setvarr_statement(struct basic_ctx* ctx);
void setvars_statement(struct basic_ctx* ctx);

/* Games launcher */
int64_t basic_game(struct basic_ctx* ctx);
