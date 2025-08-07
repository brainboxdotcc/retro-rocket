/**
 * @file basic/array.h
 * @brief Header for array manipulation functions in the BASIC interpreter
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Set the value of a specific index in an integer array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct basic_ctx* ctx);

/**
 * @brief Set the value of a specific index in a string array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_string_array_variable(const char* var, int64_t index, const char* value, struct basic_ctx* ctx);

/**
 * @brief Set the value of a specific index in a double array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_double_array_variable(const char* var, int64_t index, double value, struct basic_ctx* ctx);

/**
 * @brief Retrieve the value of a specific index in a double array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param ctx The BASIC context.
 * @param ret The retrieved value.
 * @return True if successful, false otherwise.
 */
bool basic_get_double_array_variable(const char* var, int64_t index, struct basic_ctx* ctx, double* ret);

/**
 * @brief Retrieve the value of a specific index in an integer array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param ctx The BASIC context.
 * @return The value at the specified index.
 */
int64_t basic_get_int_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);

/**
 * @brief Retrieve the value of a specific index in a string array variable.
 *
 * @param var The variable name.
 * @param index The index in the array.
 * @param ctx The BASIC context.
 * @return The value at the specified index.
 */
const char* basic_get_string_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);

/**
 * @brief Dimension a new string array with the specified size.
 *
 * @param var The variable name.
 * @param size The size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully dimensioned, false otherwise.
 */
bool basic_dim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Dimension a new integer array with the specified size.
 *
 * @param var The variable name.
 * @param size The size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully dimensioned, false otherwise.
 */
bool basic_dim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Dimension a new double array with the specified size.
 *
 * @param var The variable name.
 * @param size The size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully dimensioned, false otherwise.
 */
bool basic_dim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Resize an existing string array to the specified size.
 *
 * @param var The variable name.
 * @param size The new size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully resized, false otherwise.
 */
bool basic_redim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Resize an existing integer array to the specified size.
 *
 * @param var The variable name.
 * @param size The new size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully resized, false otherwise.
 */
bool basic_redim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Resize an existing double array to the specified size.
 *
 * @param var The variable name.
 * @param size The new size of the array.
 * @param ctx The BASIC context.
 * @return True if the array was successfully resized, false otherwise.
 */
bool basic_redim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);

/**
 * @brief Check if a variable is an integer array.
 *
 * @param ctx The BASIC context.
 * @param varname The variable name.
 * @return True if the variable is an integer array, false otherwise.
 */
bool varname_is_int_array_access(struct basic_ctx* ctx, const char* varname);

/**
 * @brief Check if a variable is a string array.
 *
 * @param ctx The BASIC context.
 * @param varname The variable name.
 * @return True if the variable is a string array, false otherwise.
 */
bool varname_is_string_array_access(struct basic_ctx* ctx, const char* varname);

/**
 * @brief Check if a variable is a double array.
 *
 * @param ctx The BASIC context.
 * @param varname The variable name.
 * @return True if the variable is a double array, false otherwise.
 */
bool varname_is_double_array_access(struct basic_ctx* ctx, const char* varname);

/**
 * @brief Get the array index from an expression.
 *
 * @param ctx The BASIC context.
 * @return The index of the array.
 */
int64_t arr_variable_index(struct basic_ctx* ctx);

/**
 * @brief Set the value of all elements in an integer array to a specified value.
 *
 * @param var The variable name.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_int_array(const char* var, int64_t value, struct basic_ctx* ctx);

/**
 * @brief Set the value of all elements in a string array to a specified value.
 *
 * @param var The variable name.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_string_array(const char* var, const char* value, struct basic_ctx* ctx);

/**
 * @brief Set the value of all elements in a double array to a specified value.
 *
 * @param var The variable name.
 * @param value The value to set.
 * @param ctx The BASIC context.
 */
void basic_set_double_array(const char* var, double value, struct basic_ctx* ctx);

/**
 * @brief Handle the `DIM` statement to define the size of an array.
 *
 * @param ctx The BASIC context.
 */
void dim_statement(struct basic_ctx* ctx);

/**
 * @brief Handle the `REDIM` statement to resize an existing array.
 *
 * @param ctx The BASIC context.
 */
void redim_statement(struct basic_ctx* ctx);

/**
 * @brief Handle the `POP` statement to remove an element from an array.
 *
 * @param ctx The BASIC context.
 */
void pop_statement(struct basic_ctx* ctx);

/**
 * @brief Handle the `PUSH` statement to insert an element into an array.
 *
 * @param ctx The BASIC context.
 */
void push_statement(struct basic_ctx* ctx);

/**
 * @brief Set the array index in an expression.
 *
 * @param ctx The BASIC context.
 * @param varname The variable name.
 * @return The array index.
 */
int64_t arr_expr_set_index(struct basic_ctx* ctx, const char* varname);

/**
 * @brief Remove an element from a string array at a specific position.
 *
 * @param var The variable name.
 * @param pop_pos The position in the array to remove from.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_pop_string_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);

/**
 * @brief Remove an element from an integer array at a specific position.
 *
 * @param var The variable name.
 * @param pop_pos The position in the array to remove from.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_pop_int_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);

/**
 * @brief Remove an element from a double array at a specific position.
 *
 * @param var The variable name.
 * @param pop_pos The position in the array to remove from.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_pop_double_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);

/**
 * @brief Insert a new element into a string array at a specific position.
 *
 * @param var The variable name.
 * @param push_pos The position in the array to insert at.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_push_string_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);

/**
 * @brief Insert a new element into an integer array at a specific position.
 *
 * @param var The variable name.
 * @param push_pos The position in the array to insert at.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_push_int_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);

/**
 * @brief Insert a new element into a double array at a specific position.
 *
 * @param var The variable name.
 * @param push_pos The position in the array to insert at.
 * @param ctx The BASIC context.
 * @return True if successful, false otherwise.
 */
bool basic_push_double_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);
