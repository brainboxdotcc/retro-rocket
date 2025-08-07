#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Retrieves the value of an integer variable by name.
 *
 * This function gets the value of a BASIC integer variable from the context.
 *
 * @param ctx The current BASIC context.
 * @return The value of the integer variable.
 */
int64_t basic_getvar_int(struct basic_ctx* ctx);

/**
 * @brief Retrieves the value of a real (double) variable by name.
 *
 * This function retrieves the value of a real (double) variable from the context.
 *
 * @param ctx The current BASIC context.
 * @param res A pointer to store the resulting real value.
 */
void basic_getvar_real(struct basic_ctx* ctx, double* res);

/**
 * @brief Retrieves the value of a string variable by name.
 *
 * This function retrieves the value of a string variable from the context.
 *
 * @param ctx The current BASIC context.
 * @return The value of the string variable.
 */
char* basic_getvar_string(struct basic_ctx* ctx);

/**
 * @brief Checks if an integer variable exists in the context.
 *
 * This function checks if a given integer variable exists in the current BASIC context.
 *
 * @param ctx The current BASIC context.
 * @return Non-zero if the variable exists, zero otherwise.
 */
int64_t basic_existsvar_int(struct basic_ctx* ctx);

/**
 * @brief Checks if a real (double) variable exists in the context.
 *
 * This function checks if a given real (double) variable exists in the current BASIC context.
 *
 * @param ctx The current BASIC context.
 * @return Non-zero if the variable exists, zero otherwise.
 */
int64_t basic_existsvar_real(struct basic_ctx* ctx);

/**
 * @brief Checks if a string variable exists in the context.
 *
 * This function checks if a given string variable exists in the current BASIC context.
 *
 * @param ctx The current BASIC context.
 * @return Non-zero if the variable exists, zero otherwise.
 */
int64_t basic_existsvar_string(struct basic_ctx* ctx);

/**
 * @brief Sets an integer variable in the current context.
 *
 * This statement sets the value of an integer variable with optional local and global scopes.
 *
 * @param ctx The current BASIC context.
 */
void setvari_statement(struct basic_ctx* ctx);

/**
 * @brief Sets a real (double) variable in the current context.
 *
 * This statement sets the value of a real (double) variable with optional local and global scopes.
 *
 * @param ctx The current BASIC context.
 */
void setvarr_statement(struct basic_ctx* ctx);

/**
 * @brief Sets a string variable in the current context.
 *
 * This statement sets the value of a string variable with optional local and global scopes.
 *
 * @param ctx The current BASIC context.
 */
void setvars_statement(struct basic_ctx* ctx);
