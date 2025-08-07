#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <basic.h>

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
	 * @brief True if the program has claimed flipping
	 */
	bool claimed_flip;
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
	/**
	 * @brief Sprites
	 */
	sprite_t* sprites[MAX_SPRITES];
	/**
	 * @brief Storage for GC'd strings
	 */
	char* string_gc_storage;
	/**
	 * @brief Next ptr for GC'd strings
	 */
	char* string_gc_storage_next;
	/**
	 * @brief Buddy allocator to contain the program's heap
	 */
	buddy_allocator_t* allocator;
} basic_ctx;

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
void set_system_variables(struct basic_ctx* ctx, uint32_t pid);
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
void basic_free_defs(struct basic_ctx* ctx);
char* printable_syntax(struct basic_ctx* ctx);
void library_statement(struct basic_ctx* ctx);
