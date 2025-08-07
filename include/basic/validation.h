#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Checks if the given variable name is valid for an integer variable.
 *
 * This function validates if the provided name conforms to the rules for an integer variable in the BASIC context.
 *
 * @param name The variable name to check.
 * @return True if the variable name is valid for an integer, false otherwise.
 */
bool valid_int_var(const char* name);

/**
 * @brief Checks if the given variable name is valid for a string variable.
 *
 * This function validates if the provided name conforms to the rules for a string variable in the BASIC context.
 *
 * @param name The variable name to check.
 * @return True if the variable name is valid for a string, false otherwise.
 */
bool valid_string_var(const char* name);

/**
 * @brief Checks if the given variable name is valid for a double (real) variable.
 *
 * This function validates if the provided name conforms to the rules for a double (real) variable in the BASIC context.
 *
 * @param name The variable name to check.
 * @return True if the variable name is valid for a double, false otherwise.
 */
bool valid_double_var(const char* name);
