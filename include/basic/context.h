/**
 * @file basic/context.h
 * @brief Functions related to BASIC context management, program execution, and function handling
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "buddy_allocator.h"


/**
 * @brief BASIC program context.
 *
 * Every instance of a BASIC program has one of these contexts. Certain structures,
 * such as functions, will clone the context and run on the clone until the function
 * completes. Cloned contexts share variables, and you should never call basic_destroy()
 * on them as they are handled differently.
 */
typedef struct basic_ctx {
	/**
	 * @brief Pointer to the start of the next token.
	 *
	 * This pointer always lies between `program_ptr` and `program_ptr + strlen(program_ptr)`.
	 */
	char const* ptr;

	/**
	 * @brief Pointer to the character after the next token.
	 *
	 * This pointer always lies between `program_ptr` and `program_ptr + strlen(program_ptr)`.
	 */
	char const* nextptr;

	/**
	 * @brief Current token type.
	 *
	 * A numeric representation of the token between `ptr` and `nextptr`. The value
	 * should always be within the `token_t` enum.
	 */
	int current_token;

	/**
	 * @brief Current line number in the program.
	 */
	int64_t current_linenum;

	/**
	 * @brief True if the program has thrown an error and should terminate.
	 *
	 * This flag might not cause immediate termination if the program is inside an `EVAL`.
	 */
	bool errored;

	/**
	 * @brief True if the program has ended.
	 *
	 * This occurs when the program reaches an explicit `END` statement or falls off the
	 * end of the program, triggering termination.
	 */
	bool ended;

	/**
	 * @brief If non-null, points at the error handling procedure
	 * for the program. In the event of an error, instead of terminating the program,
	 * the error handling procedure is called instead. After the error is handled,
	 * the line after the error is executed.
	 */
	const char* error_handler;

	/**
	 * @brief True if the program has "claimed flipping".
	 *
	 * This is relevant to the internal state of the program's execution.
	 */
	bool claimed_flip;

	/**
	 * @brief Pointer to the program's entire text (untokenized).
	 *
	 * This may have been processed by a preprocessing phase to remove unnecessary spacing, etc.
	 */
	char* program_ptr;

	/**
	 * @brief Debugging status flag (e.g., `DEBUG_*` values).
	 *
	 * Holds information about the current debugging state.
	 */
	uint8_t debug_status;

	/**
	 * @brief Array of active debug breakpoints (line numbers).
	 *
	 * Used during program execution for debugging purposes.
	 */
	uint64_t* debug_breakpoints;

	/**
	 * @brief Number of active breakpoints.
	 */
	uint64_t debug_breakpoint_count;

	/**
	 * @brief Local buffer for parsing function/procedure parameter lists.
	 */
	char string[MAX_STRINGLEN];

	/**
	 * @brief Local integer variable stack for function/procedure scopes.
	 *
	 * Each index in this array corresponds to a specific depth in the call stack.
	 */
	struct ub_var_int* local_int_variables[MAX_CALL_STACK_DEPTH];

	/**
	 * @brief Local string variable stack for function/procedure scopes.
	 */
	struct ub_var_string* local_string_variables[MAX_CALL_STACK_DEPTH];

	/**
	 * @brief Local double (real) variable stack for function/procedure scopes.
	 */
	struct ub_var_double* local_double_variables[MAX_CALL_STACK_DEPTH];

	/**
	 * @brief Call stack for return line numbers during function, procedure, or `GOSUB` calls.
	 *
	 * Each index corresponds to a call in the stack, with a maximum depth of `MAX_CALL_STACK_DEPTH`.
	 */
	uint64_t call_stack[MAX_CALL_STACK_DEPTH];

	/**
	 * @brief Pointer indicating the current position in the call stack.
	 *
	 * Starts at 0 and can go up to `MAX_CALL_STACK_DEPTH - 1`.
	 */
	uint64_t call_stack_ptr;

	/**
	 * @brief Repeat stack for handling `REPEAT...UNTIL` loop control.
	 *
	 * Holds the return line numbers for each level of loop nesting.
	 */
	uint64_t repeat_stack[MAX_LOOP_STACK_DEPTH];

	/**
	 * @brief Pointer indicating the current position in the repeat stack.
	 */
	uint64_t repeat_stack_ptr;

	/**
	 * @brief While stack for handling `WHILE...ENDWHILE` loop control.
	 *
	 * Holds the return line numbers for each level of loop nesting.
	 */
	uint64_t while_stack[MAX_LOOP_STACK_DEPTH];

	/**
	 * @brief Pointer indicating the current position in the repeat stack.
	 */
	uint64_t while_stack_ptr;

	/**
	 * @brief Previous length of the program text, used to detect an `EVAL` in progress.
	 *
	 * If this value is non-zero, an `EVAL` is in progress, and the program has appended
	 * additional lines. If zero, no `EVAL` is executing.
	 */
	size_t oldlen;

	/**
	 * @brief The return line number for an `EVAL` statement.
	 */
	int64_t eval_linenum;

	/**
	 * @brief FOR loop stack to handle `FOR...NEXT` loops.
	 *
	 * Contains return line numbers for each level of loop.
	 */
	struct for_state for_stack[MAX_LOOP_STACK_DEPTH];

	/**
	 * @brief Pointer indicating the current position in the FOR loop stack.
	 */
	uint64_t for_stack_ptr;

	/**
	 * @brief Definitions of procedures and functions in the program.
	 *
	 * Linked list of all function and procedure definitions.
	 */
	struct ub_proc_fn_def* defs;

	/**
	 * @brief Global integer variable list.
	 *
	 * Stores all globally scoped integer variables in the program.
	 */
	struct ub_var_int* int_variables;

	/**
	 * @brief Global string variable list.
	 */
	struct ub_var_string* str_variables;

	/**
	 * @brief Global double variable list.
	 */
	struct ub_var_double* double_variables;

	/**
	 * @brief Global integer array variable list.
	 */
	struct ub_var_int_array* int_array_variables;

	/**
	 * @brief Global string array variable list.
	 */
	struct ub_var_string_array* string_array_variables;

	/**
	 * @brief Global double array variable list.
	 */
	struct ub_var_double_array* double_array_variables;

	/**
	 * @brief Expected return type of the function being executed.
	 *
	 * This is used when executing a function atomically to determine the type of the return value.
	 */
	ub_return_type fn_type;

	/**
	 * @brief Pointer to the return value of the function being executed.
	 *
	 * This is used for storing the result of a function's execution.
	 */
	void* fn_return;

	/**
	 * @brief Bracket depth when parsing function or procedure parameter lists.
	 *
	 * Keeps track of nested parentheses during parameter parsing.
	 */
	int bracket_depth;

	/**
	 * @brief Item start pointer when parsing function or procedure parameters.
	 */
	char const* item_begin;

	/**
	 * @brief Linked list of parameters for functions and procedures.
	 *
	 * This is used when parsing parameter lists in function/procedure calls.
	 */
	struct ub_param* param;

	/**
	 * @brief Current graphics color for graphical operations (e.g., drawing lines, shapes).
	 */
	int32_t graphics_colour;

	/**
	 * @brief The last CPUID instruction result, used for system information.
	 */
	cpuid_result_t last_cpuid_result;

	/**
	 * @brief Hashmap for quick lookup of line numbers in the program.
	 *
	 * Provides efficient access to the program's lines for execution.
	 */
	struct hashmap* lines;

	/**
	 * @brief Current depth of block `IF...THEN...ELSE` statements.
	 *
	 * This is used for managing nested `IF` statements and ensuring correct execution flow.
	 */
	uint64_t if_nest_level;

	/**
	 * @brief Highest line number in the program.
	 *
	 * Keeps track of the largest line number in the program for reference.
	 */
	int64_t highest_line;

	/**
	 * @brief Array of sprites used in the program (if any).
	 */
	sprite_t* sprites[MAX_SPRITES];

	/**
	 * @brief Storage area for garbage-collected strings.
	 *
	 * This holds strings that are subject to garbage collection.
	 */
	char* string_gc_storage;

	/**
	 * @brief Pointer to the next free position in the garbage-collected string storage.
	 */
	char* string_gc_storage_next;

	/**
	 * @brief Buddy allocator used for managing the program's heap.
	 *
	 * The allocator is used to manage dynamic memory for the program's variables and data structures.
	 */
	buddy_allocator_t* allocator;
} basic_ctx;

/**
 * @brief Initialize the BASIC context with the provided program, console, process ID, and file.
 *
 * @param program The program code to execute.
 * @param cons The console to use for input/output.
 * @param pid The process ID.
 * @param file The file containing the program.
 * @param error Pointer to store any error messages.
 * @return A pointer to the initialized `basic_ctx` context.
 */
struct basic_ctx* basic_init(const char *program, uint32_t pid, const char* file, char** error);

/**
 * @brief Destroy the BASIC context and free associated resources.
 *
 * @param ctx The BASIC context to destroy.
 */
void basic_destroy(struct basic_ctx* ctx);

/**
 * @brief Run the BASIC program by executing the next statement.
 *
 * @param ctx The BASIC context.
 */
void basic_run(struct basic_ctx* ctx);

/**
 * @brief Check if the BASIC program has finished executing.
 *
 * @param ctx The BASIC context.
 * @return True if the program has finished, false otherwise.
 */
bool basic_finished(struct basic_ctx* ctx);

/**
 * @brief Jump to a specific line number in the BASIC program.
 *
 * @param linenum The line number to jump to.
 * @param ctx The BASIC context.
 * @return True if the jump was successful, false if the line doesn't exist.
 */
bool jump_linenum(int64_t linenum, struct basic_ctx* ctx);

/**
 * @brief Parse and execute a line in the BASIC program.
 *
 * @param ctx The BASIC context.
 */
void line_statement(struct basic_ctx* ctx);

/**
 * @brief Parse and execute a statement in the BASIC program.
 *
 * @param ctx The BASIC context.
 */
void statement(struct basic_ctx* ctx);

/**
 * @brief Check if the expected token is the current token in the BASIC program.
 *
 * @param token The token to check for.
 * @param ctx The BASIC context.
 * @return True if the token matches, false otherwise.
 */
bool accept(int token, struct basic_ctx* ctx);

/**
 * @brief Parse function definitions in the BASIC program.
 *
 * @param ctx The BASIC context.
 * @return True if the function definitions were parsed successfully, false otherwise.
 */
bool basic_parse_fn(struct basic_ctx* ctx);

/**
 * @brief Clone an existing BASIC context to create a new one.
 *
 * @param old The original BASIC context.
 * @return A new `basic_ctx` that is a clone of the original.
 */
struct basic_ctx* basic_clone(struct basic_ctx* old);

/**
 * @brief Set system variables in the BASIC context.
 *
 * @param ctx The BASIC context.
 * @param pid The process ID.
 */
void set_system_variables(struct basic_ctx* ctx, uint32_t pid);

/**
 * @brief Check if a function name corresponds to a built-in string function.
 *
 * @param fn_name The name of the function to check.
 * @param ctx The BASIC context.
 * @param res Pointer to store the result of the function if found.
 * @return 1 if the function is found and executed, 0 otherwise.
 */
char basic_builtin_str_fn(const char* fn_name, struct basic_ctx* ctx, char** res);

/**
 * @brief Evaluate an integer function in the context of BASIC.
 *
 * @param fn_name The name of the function to evaluate.
 * @param ctx The BASIC context.
 * @return The evaluated integer result of the function.
 */
int64_t basic_eval_int_fn(const char* fn_name, struct basic_ctx* ctx);

/**
 * @brief Evaluate a double (floating-point) function in the context of BASIC.
 *
 * @param fn_name The name of the function to evaluate.
 * @param ctx The BASIC context.
 * @param res Pointer to store the evaluated result of the function.
 */
void basic_eval_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res);

/**
 * @brief Evaluate a string function in the context of BASIC.
 *
 * @param fn_name The name of the function to evaluate.
 * @param ctx The BASIC context.
 * @return The evaluated string result of the function.
 */
const char* basic_eval_str_fn(const char* fn_name, struct basic_ctx* ctx);

/**
 * @brief Check if a function name corresponds to a built-in double (floating-point) function.
 *
 * @param fn_name The name of the function to check.
 * @param ctx The BASIC context.
 * @param res Pointer to store the result of the function if found.
 * @return 1 if the function is found and executed, 0 otherwise.
 */
char basic_builtin_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res);

/**
 * @brief Check if a function name corresponds to a built-in integer function.
 *
 * @param fn_name The name of the function to check.
 * @param ctx The BASIC context.
 * @param res Pointer to store the result of the function if found.
 * @return 1 if the function is found and executed, 0 otherwise.
 */
char basic_builtin_int_fn(const char* fn_name, struct basic_ctx* ctx, int64_t* res);

/**
 * @brief Find a function definition by name in the BASIC context.
 *
 * @param name The name of the function to find.
 * @param ctx The BASIC context.
 * @return The function definition if found, NULL otherwise.
 */
struct ub_proc_fn_def* basic_find_fn(const char* name, struct basic_ctx* ctx);

/**
 * @brief Initialize the local call stack for the current function or procedure.
 *
 * @param ctx The BASIC context.
 */
void init_local_heap(struct basic_ctx* ctx);

/**
 * @brief Free the local call stack and associated variables for the current function or procedure.
 *
 * @param ctx The BASIC context.
 */
void free_local_heap(struct basic_ctx* ctx);

/**
 * @brief Check if a function name corresponds to a built-in double (floating-point) function.
 *
 * @param fn_name The name of the function to check.
 * @return True if the function is a built-in double function, false otherwise.
 */
bool is_builtin_double_fn(const char* fn_name);

/**
 * @brief Free function definitions and associated resources in the BASIC context.
 *
 * @param ctx The BASIC context.
 */
void basic_free_defs(struct basic_ctx* ctx);

/**
 * @brief Get the printable syntax of an expression in the BASIC program.
 *
 * @param ctx The BASIC context.
 * @return The printable syntax as a string.
 */
char* printable_syntax(struct basic_ctx* ctx);

/**
 * @brief Handle the LIBRARY statement, which loads a library into the program.
 *
 * @param ctx The BASIC context.
 */
void library_statement(struct basic_ctx* ctx);


