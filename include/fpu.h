/**
 * @file fpu.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

/**
 * @brief Enable floating point operations
 */
extern void enable_fpu();

/**
 * @brief Enable SSE
 * @warning Do not use, this is not stable!
 */
extern void enable_sse();

/**
 * @brief Determine the approximate number of decimal places
 * in a double value. This uses a very simple loop which
 * repeatedly multiplies the value by ten, until all that
 * remains is a whole number greater than or equal to 1.
 * 
 * @param f double to count decimal places
 * @return uint8_t number of decimal places
 */
uint8_t double_determine_decimal_places(double f);

/**
 * @brief Convert a double number to char* string,
 * with a specified decimal precision.
 *
 * This function retains accuraccy by converting to 64 bit
 * integers and string representations as early as possible,
 * then operating on fixed-point representations of the
 * floating point number. When we deal with fixed point, we
 * don't lose accuraccy due to rounding.
 * 
 * @param x Float to convert
 * @param p Output buffer
 * @param len Size of output buffer
 * @param precision Precision of decimal part. Set to 0 to
 * display full stored precision of the value.
 * @return char* pointer to value in buffer 
 */
char* double_to_string(double x, char *p, int64_t len, uint8_t precision);