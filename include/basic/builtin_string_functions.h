#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Returns the leftmost part of a string.
 *
 * This function returns a substring from the leftmost part of a string, up to the specified
 * number of characters. If the specified length is greater than the string's length, it will return
 * the entire string.
 *
 * @param ctx BASIC interpreter context
 * @return A substring from the leftmost part of the string
 */
char* basic_left(struct basic_ctx* ctx);

/**
 * @brief Returns the rightmost part of a string.
 *
 * This function returns a substring from the rightmost part of a string, up to the specified
 * number of characters. If the specified length is greater than the string's length, it will return
 * the entire string.
 *
 * @param ctx BASIC interpreter context
 * @return A substring from the rightmost part of the string
 */
char* basic_right(struct basic_ctx* ctx);

/**
 * @brief Returns a substring from the middle of a string.
 *
 * This function extracts a substring starting from a specified position within a string and
 * continuing for a specified length.
 *
 * @param ctx BASIC interpreter context
 * @return A substring from the middle of the string
 */
char* basic_mid(struct basic_ctx* ctx);

/**
 * @brief Returns the ASCII character for a given number.
 *
 * This function returns the ASCII character that corresponds to the specified integer value.
 * If the integer is not a valid ASCII value, the function may return undefined results.
 *
 * @param ctx BASIC interpreter context
 * @return The ASCII character corresponding to the given number
 */
char* basic_chr(struct basic_ctx* ctx);

/**
 * @brief Reads a string from input.
 *
 * This function reads a string from user input or a defined source. It is typically used for
 * getting string input from the user during program execution.
 *
 * @param ctx BASIC interpreter context
 * @return The string read from input
 */
char* basic_readstring(struct basic_ctx* ctx);

/**
 * @brief Converts a string to uppercase.
 *
 * This function converts all alphabetic characters in the string to their uppercase equivalents.
 * Non-alphabetic characters remain unchanged.
 *
 * @param ctx BASIC interpreter context
 * @return The uppercase version of the input string
 */
char* basic_upper(struct basic_ctx* ctx);

/**
 * @brief Converts a string to lowercase.
 *
 * This function converts all alphabetic characters in the string to their lowercase equivalents.
 * Non-alphabetic characters remain unchanged.
 *
 * @param ctx BASIC interpreter context
 * @return The lowercase version of the input string
 */
char* basic_lower(struct basic_ctx* ctx);

/**
 * @brief Tokenizes a string based on a delimiter.
 *
 * This function splits a string into tokens, using a specified delimiter. Each call to the function
 * returns the next token from the string.
 *
 * @param ctx BASIC interpreter context
 * @return The next token from the string
 */
char* basic_tokenize(struct basic_ctx* ctx);

/**
 * @brief Gets the current selected directory (like getcwd in C).
 *
 * This function returns the path of the current working directory, which is the directory where the
 * program is currently operating from.
 *
 * @param ctx BASIC interpreter context
 * @return The path of the current working directory
 */
char* basic_csd(struct basic_ctx* ctx);

/**
 * @brief Gets the brand of the CPU.
 *
 * This function returns the brand of the CPU, providing information about the specific CPU model
 * that the system is running.
 *
 * @param ctx BASIC interpreter context
 * @return The brand of the CPU
 */
char* basic_cpugetbrand(struct basic_ctx* ctx);

/**
 * @brief Gets the vendor of the CPU.
 *
 * This function returns the vendor of the CPU, providing information about the manufacturer
 * of the CPU.
 *
 * @param ctx BASIC interpreter context
 * @return The vendor of the CPU
 */
char* basic_cpugetvendor(struct basic_ctx* ctx);

/**
 * @brief Converts a number to its ASCII string representation.
 *
 * This function converts a number into its ASCII equivalent. For example, the number 123 would be
 * converted to the string "123".
 *
 * @param ctx BASIC interpreter context
 * @return The ASCII string representation of the number
 */
char* basic_intoasc(struct basic_ctx* ctx);

/**
 * @brief Left-justifies a string.
 *
 * This function left-justifies a string by padding it with spaces to a specified length.
 * If the string is longer than the specified length, it will be truncated.
 *
 * @param ctx BASIC interpreter context
 * @return The left-justified string
 */
char* basic_ljust(struct basic_ctx* ctx);

/**
 * @brief Right-justifies a string.
 *
 * This function right-justifies a string by padding it with spaces to a specified length.
 * If the string is longer than the specified length, it will be truncated.
 *
 * @param ctx BASIC interpreter context
 * @return The right-justified string
 */
char* basic_rjust(struct basic_ctx* ctx);

/**
 * @brief Removes leading whitespace from a string.
 *
 * This function removes any spaces or other whitespace characters from the beginning of the string.
 *
 * @param ctx BASIC interpreter context
 * @return The string with leading whitespace removed
 */
char* basic_ltrim(struct basic_ctx* ctx);

/**
 * @brief Removes trailing whitespace from a string.
 *
 * This function removes any spaces or other whitespace characters from the end of the string.
 *
 * @param ctx BASIC interpreter context
 * @return The string with trailing whitespace removed
 */
char* basic_rtrim(struct basic_ctx* ctx);

/**
 * @brief Removes both leading and trailing whitespace from a string.
 *
 * This function removes any spaces or other whitespace characters from both the beginning and
 * the end of the string.
 *
 * @param ctx BASIC interpreter context
 * @return The string with both leading and trailing whitespace removed
 */
char* basic_trim(struct basic_ctx* ctx);

/**
 * @brief Converts an integer to a string.
 *
 * This function converts an integer into its string representation.
 *
 * @param ctx BASIC interpreter context
 * @return The string representation of the integer
 */
char* basic_itoa(struct basic_ctx* ctx);

/**
 * @brief Repeats a string a specified number of times.
 *
 * This function repeats a string multiple times, as specified by the second parameter.
 *
 * @param ctx BASIC interpreter context
 * @return The repeated string
 */
char* basic_repeat(struct basic_ctx* ctx);

/**
 * @brief Reverses the characters in a string.
 *
 * This function reverses the characters in a given string.
 *
 * @param ctx BASIC interpreter context
 * @return The reversed string
 */
char* basic_reverse(struct basic_ctx* ctx);

/**
 * @brief Returns the string representation of a value.
 *
 * This function returns the string equivalent of the given value, whether it's a string,
 * number, or other type.
 *
 * @param ctx BASIC interpreter context
 * @return The string representation of the value
 */
char* basic_str(struct basic_ctx* ctx);

/**
 * @brief Converts a boolean value to a string.
 *
 * This function converts a boolean value (true or false) to its string representation ("True" or "False").
 *
 * @param ctx BASIC interpreter context
 * @return The string representation of the boolean value
 */
char* basic_bool(struct basic_ctx* ctx);

char* basic_replace(struct basic_ctx* ctx);

char* basic_highlight(struct basic_ctx* ctx);