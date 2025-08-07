#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Builtin integer functions
 */
/**
 * @brief Calculates the absolute value of a number.
 *
 * This function returns the absolute value of the input number. If the number is
 * negative, it will return the positive equivalent, and if the number is positive,
 * it will return the same value.
 *
 * @param ctx BASIC interpreter context
 * @return The absolute value of the number
 */
int64_t basic_abs(struct basic_ctx* ctx);

/**
 * @brief Returns the length of a string.
 *
 * This function returns the number of characters in the provided string variable.
 * It will count the number of characters excluding the null-terminator.
 *
 * @param ctx BASIC interpreter context
 * @return The length of the string
 */
int64_t basic_len(struct basic_ctx* ctx);

/**
 * @brief Finds the position of a substring within a string.
 *
 * This function searches for the first occurrence of a substring within another string
 * and returns the position (1-based index) where it is found. If the substring is not found,
 * the function returns 0.
 *
 * @param ctx BASIC interpreter context
 * @return The position of the substring (1-based index)
 */
int64_t basic_instr(struct basic_ctx* ctx);

/**
 * @brief Returns the ASCII value of the first character of a string.
 *
 * This function takes a string and returns the ASCII value of its first character.
 * If the string is empty, it returns 0.
 *
 * @param ctx BASIC interpreter context
 * @return The ASCII value of the first character of the string
 */
int64_t basic_asc(struct basic_ctx* ctx);

/**
 * @brief Converts RGB values to a single integer representing the color.
 *
 * This function takes the red, green, and blue components of a color and returns an integer
 * that represents the combined color value.
 *
 * @param ctx BASIC interpreter context
 * @return A combined integer representing the RGB color
 */
int64_t basic_rgb(struct basic_ctx* ctx);

/**
 * @brief Generates a random number.
 *
 * This function generates a random number between 0 and 32767 (inclusive). It can be used
 * to introduce randomness into the program.
 *
 * @param ctx BASIC interpreter context
 * @return A random integer between 0 and 32767
 */
int64_t basic_random(struct basic_ctx* ctx);

/**
 * @brief Converts a string to an integer.
 *
 * This function converts the provided string representation of a number to its integer
 * equivalent. It will ignore any leading spaces and handle the conversion from decimal format.
 *
 * @param ctx BASIC interpreter context
 * @return The integer equivalent of the string
 */
int64_t basic_atoi(struct basic_ctx* ctx);

/**
 * @brief Shifts a number left by a specified number of bits.
 *
 * This function performs a bitwise left shift operation on an integer by the number of bits
 * specified by the second operand. It is equivalent to multiplying the number by 2 for each
 * bit position shifted.
 *
 * @param ctx BASIC interpreter context
 * @return The result of the left shift operation
 */
int64_t basic_shl(struct basic_ctx* ctx);

/**
 * @brief Shifts a number right by a specified number of bits.
 *
 * This function performs a bitwise right shift operation on an integer by the number of bits
 * specified by the second operand. It is equivalent to dividing the number by 2 for each
 * bit position shifted.
 *
 * @param ctx BASIC interpreter context
 * @return The result of the right shift operation
 */
int64_t basic_shr(struct basic_ctx* ctx);

/**
 * @brief Returns the sign of a number.
 *
 * This function returns 1 if the number is positive, -1 if it is negative, and 0 if it is zero.
 * It can be used to determine whether a number is positive, negative, or zero.
 *
 * @param ctx BASIC interpreter context
 * @return The sign of the number (1, -1, or 0)
 */
int64_t basic_sgn(struct basic_ctx* ctx);

/**
 * @brief Rounds a number to the nearest integer.
 *
 * This function rounds the provided number to the nearest integer. If the number is halfway between
 * two integers, it rounds up to the next higher integer.
 *
 * @param ctx BASIC interpreter context
 * @return The rounded integer value
 */
int64_t basic_int(struct basic_ctx* ctx);

/**
 * @brief Converts a string to its numerical value.
 *
 * This function converts the provided string representation of a number to its numerical
 * equivalent (either integer or floating point).
 *
 * @param ctx BASIC interpreter context
 * @return The numerical value of the string
 */
int64_t basic_val(struct basic_ctx* ctx);

/**
 * @brief Converts a hexadecimal string to its integer value.
 *
 * This function converts the provided hexadecimal string to its integer equivalent.
 * The string should represent a valid hexadecimal number, optionally prefixed with "&".
 *
 * @param ctx BASIC interpreter context
 * @return The integer equivalent of the hexadecimal string
 */
int64_t basic_hexval(struct basic_ctx* ctx);

/**
 * @brief Converts an octal string to its integer value.
 *
 * This function converts the provided octal string to its integer equivalent.
 * The string should represent a valid octal number.
 *
 * @param ctx BASIC interpreter context
 * @return The integer equivalent of the octal string
 */
int64_t basic_octval(struct basic_ctx* ctx);
