#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Get the value of an integer variable.
 *
 * This function retrieves the value of an integer variable in the BASIC context.
 *
 * @param varname The name of the integer variable.
 * @param ctx The current BASIC context.
 * @return The value of the integer variable.
 */
int64_t basic_get_int_variable(const char* varname, struct basic_ctx* ctx);

/**
 * @brief Get the value of a double (real) variable.
 *
 * This function retrieves the value of a double variable in the BASIC context.
 *
 * @param var The name of the double variable.
 * @param ctx The current BASIC context.
 * @param res Pointer to store the result of the double value.
 * @return True if the variable was found and the value was retrieved, false otherwise.
 */
bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res);

/**
 * @brief Get the value of a string variable.
 *
 * This function retrieves the value of a string variable in the BASIC context.
 *
 * @param var The name of the string variable.
 * @param ctx The current BASIC context.
 * @return The value of the string variable.
 */
const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx);

/**
 * @brief Set the value of a string variable.
 *
 * This function sets the value of a string variable in the BASIC context.
 *
 * @param var The name of the string variable.
 * @param value The value to set for the string variable.
 * @param ctx The current BASIC context.
 * @param local Boolean indicating whether the variable is local.
 * @param propagate_global Boolean indicating whether the variable is propagated globally to child programs.
 */
void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool propagate_global);

/**
 * @brief Set the value of a double (real) variable.
 *
 * This function sets the value of a double variable in the BASIC context.
 *
 * @param var The name of the double variable.
 * @param value The value to set for the double variable.
 * @param ctx The current BASIC context.
 * @param local Boolean indicating whether the variable is local.
 * @param propagate_global Boolean indicating whether the variable is propagated globally to child programs.
 */
void basic_set_double_variable(const char* var, const double value, struct basic_ctx* ctx, bool local, bool propagate_global);

/**
 * @brief Set the value of an integer variable.
 *
 * This function sets the value of an integer variable in the BASIC context.
 *
 * @param var The name of the integer variable.
 * @param value The value to set for the integer variable.
 * @param ctx The current BASIC context.
 * @param local Boolean indicating whether the variable is local.
 * @param propagate_global Boolean indicating whether the variable is propagated globally to child programs.
 */
void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool propagate_global);

/**
 * @brief Get the numeric value of a variable.
 *
 * This function retrieves the numeric value of a variable and stores it in `res`.
 *
 * @param var The name of the variable.
 * @param ctx The current BASIC context.
 * @param res Pointer to store the result of the numeric value.
 * @return The return type for the variable (integer, string, etc.).
 */
ub_return_type basic_get_numeric_variable(const char* var, struct basic_ctx* ctx, double* res);

/**
 * @brief Get the numeric value of an integer variable.
 *
 * This function retrieves the numeric value of an integer variable.
 *
 * @param var The name of the integer variable.
 * @param ctx The current BASIC context.
 * @return The value of the integer variable.
 */
int64_t basic_get_numeric_int_variable(const char* var, struct basic_ctx* ctx);

/**
 * @brief Handle variable assignment
 *
 * This function processes the LET statement, setting the value of a variable.
 *
 * @param ctx The current BASIC context.
 * @param global Boolean indicating whether the variable is global.
 * @param local Boolean indicating whether the variable is local.
 */
void assignment_statement(struct basic_ctx* ctx, bool global, bool local);

/**
 * @brief Check if an integer variable exists.
 *
 * This function checks if the specified integer variable exists in the BASIC context.
 *
 * @param var The name of the integer variable.
 * @param ctx The current BASIC context.
 * @return True if the integer variable exists, false otherwise.
 */
bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx);

/**
 * @brief Check if a string variable exists.
 *
 * This function checks if the specified string variable exists in the BASIC context.
 *
 * @param var The name of the string variable.
 * @param ctx The current BASIC context.
 * @return True if the string variable exists, false otherwise.
 */
bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx);

/**
 * @brief Check if a double (real) variable exists.
 *
 * This function checks if the specified double (real) variable exists in the BASIC context.
 *
 * @param var The name of the double variable.
 * @param ctx The current BASIC context.
 * @return True if the double variable exists, false otherwise.
 */
bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx);

void let_statement(struct basic_ctx* ctx);

void variable_statement(struct basic_ctx* ctx);

void global_statement(struct basic_ctx* ctx);

void local_statement(struct basic_ctx* ctx);

void newline_statement(struct basic_ctx* ctx);