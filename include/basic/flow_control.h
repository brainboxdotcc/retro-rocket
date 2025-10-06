#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Handles the DEF statement in BASIC, used for function or procedure definitions.
 *
 * The DEF statement is used to define a function or a procedure in BASIC. It processes
 * the definition and parameter list.
 *
 * @param ctx The current BASIC program context.
 */
void def_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the PROC statement in BASIC, used for calling a procedure.
 *
 * The PROC statement calls a procedure that has been previously defined.
 *
 * @param ctx The current BASIC program context.
 */
void proc_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the ENDPROC statement in BASIC, used for returning from a procedure.
 *
 * The ENDPROC statement is used to return control from a procedure to the calling location.
 *
 * @param ctx The current BASIC program context.
 */
void endproc_statement(struct basic_ctx* ctx);

/**
 * @brief Extracts a parameter from a comma-separated list of parameters for a FN/PROC call.
 *
 * This function extracts individual parameters from a function's comma-separated argument list
 * and processes them based on their type.
 *
 * You should call it in a while loop:
 *
 * @code
 * int bracket_depth = 0;
 * const char* item_begin = ctx->ptr;
 * struct ub_param* param = def->params;
 * while (extract_comma_list(def, ctx, &bracket_depth, &item_begin, &param));
 * @endcode
 *
 * @param def The function definition containing the parameter list.
 * @param ctx The current BASIC program context.
 * @param bracket_depth Temporary variable used to track bracket depth in expressions
 * @param item_begin A temporary pointer to the next tokenized item
 * @param param Pointer to the linked list of parameter types and names
 * @return uint8_t 1 if more parameters are present, 0 otherwise.
 */
bool extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx, int* bracket_depth, char const** item_begin, struct ub_param** param);

/**
 * @brief Handles the = statement in BASIC, used to return a value from a FN
 *
 * The = character as a statement returns a value from a function.
 *
 * @param ctx The current BASIC program context.
 */
void eq_statement(struct basic_ctx* ctx);

/**
 * @brief Evaluates a conditional expression in BASIC, used in IF statements.
 *
 * This function evaluates a condition for an IF or other conditional statements in BASIC.
 * It checks the boolean result of the condition and returns it.
 *
 * @param ctx The current BASIC program context.
 * @return bool True if the condition is met, false otherwise.
 */
bool conditional(struct basic_ctx* ctx);

/**
 * @brief Handles the ELSE statement in BASIC, used for branching after an IF condition.
 *
 * The ELSE statement is used to execute a block of code if the corresponding IF condition is not met.
 *
 * @param ctx The current BASIC program context.
 */
void else_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the IF statement in BASIC, used for conditional execution.
 *
 * The IF statement checks a condition and, based on the result, executes either the THEN block or
 * an ELSE block.
 *
 * @param ctx The current BASIC program context.
 */
void if_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the GOSUB statement in BASIC, used for subroutine calls.
 *
 * The GOSUB statement calls a subroutine and pushes the return address to the call stack.
 *
 * @param ctx The current BASIC program context.
 */
void gosub_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the RETURN statement in BASIC, used to return from a GOSUB.
 *
 * The RETURN statement pops the return address from the call stack and returns control to that point.
 *
 * @param ctx The current BASIC program context.
 */
void return_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the NEXT statement in BASIC, used for loop iteration in FOR...NEXT.
 *
 * The NEXT statement increments the loop variable in a FOR...NEXT loop and continues the loop if the
 * condition is met, or terminates it otherwise.
 *
 * @param ctx The current BASIC program context.
 */
void next_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the FOR statement in BASIC, used to start a loop.
 *
 * The FOR statement initializes a loop with a start value, end value, and step, and jumps to the corresponding
 * loop body.
 *
 * @param ctx The current BASIC program context.
 */
void for_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the REPEAT statement in BASIC, used to start a loop.
 *
 * The REPEAT statement starts a loop that continues until the condition in the UNTIL statement is true.
 *
 * @param ctx The current BASIC program context.
 */
void repeat_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the UNTIL statement in BASIC, used to end a REPEAT loop.
 *
 * The UNTIL statement checks the loop condition and, if true, exits the REPEAT loop.
 *
 * @param ctx The current BASIC program context.
 */
void until_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the WHILE statement in BASIC, used to start a loop.
 *
 * The WHILE statement starts a loop that continues until its condition is false.
 *
 * @param ctx The current BASIC program context.
 */
void while_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the ENDWHILE statement in BASIC, used to end a WHILE loop.
 *
 * The ENDWHILE statement loops back to its corresponding WHILE loop
 *
 * @param ctx The current BASIC program context.
 */
void endwhile_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the ENDIF statement in BASIC, used to close an IF...THEN block.
 *
 * The ENDIF statement closes a block started with an IF statement, ending the conditional execution.
 *
 * @param ctx The current BASIC program context.
 */
void endif_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the END statement in BASIC, used to terminate a program.
 *
 * The END statement halts the program's execution. If inside an IF statement, it checks for a corresponding
 * ENDIF to terminate the block.
 *
 * @param ctx The current BASIC program context.
 */
void end_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the PANIC statement in BASIC, used for forceful termination of the program.
 *
 * The PANIC statement halts the execution of the program abruptly, often used for error situations.
 *
 * @param ctx The current BASIC program context.
 */
void panic_statement(struct basic_ctx* ctx);

/**
 * @brief Checks if the program is currently inside an EVAL statement.
 *
 * This function checks whether the program is inside an EVAL block, which modifies the program at runtime.
 *
 * @param ctx The current BASIC program context.
 * @return bool True if the program is inside an EVAL, false otherwise.
 */
bool basic_in_eval(struct basic_ctx* ctx);

void on_statement(struct basic_ctx* ctx);

void off_statement(struct basic_ctx* ctx);

void error_statement(struct basic_ctx* ctx);

void continue_statement(struct basic_ctx* ctx);

