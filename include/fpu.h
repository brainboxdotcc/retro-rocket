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
 * @brief Convert a floating point number to char* string,
 * with a specified decimal precision
 * 
 * @param x Float to convert
 * @param p Output buffer
 * @param len Size of output buffer
 * @param precision Precision of decimal part
 * @return char* pointer to value in buffer. Use this
 * pointer NOT the buffer to display the value, as
 * it will not point to the start of the buffer!
 */
static inline char * float_to_string(float x, char *p, size_t len, uint8_t precision) {
	if (x == 0.0) {
		*p = '0';
		*(p + 1) = 0;
		return p;
	}
	char *s = p + len - 1; // go to end of buffer
	precision = precision > 11 ? 11 : precision;
	uint64_t decimals;  // variable to store the decimals
	int64_t units;  // variable to store the units (part to left of decimal place)
	if (x < 0) { // take care of negative numbers
		decimals = (int64_t)(x * -10000000.0) % 10000000; // number of decimal places
		units = (int64_t)(-1 * x);
	} else { // positive numbers
		decimals = (int64_t)(x * 10000000.0) % 10000000;
		units = (int64_t)x;
	}
	*--s = 0; // Null terminate the string
	// We give up at p+2 leaving room for "." and one
	// digit before the decimal place
	for (int pr = 9; pr && s >= p + 2; --pr) {
		*--s = (decimals % 10) + '0';
		decimals /= 10;
		if (decimals == 0) {
			break; // No decimal left to divide by
		}
	}
	*--s = '.';
	*(s + precision + 1) = 0;

	while (units > 0 && s >= p) {
		*--s = (units % 10) + '0';
		units /= 10;
	}
	if (x < 0 && s >= p) {
		 *--s = '-'; // unary minus sign for negative numbers
	}
	return s;
}

static inline uint8_t float_determine_decimal_places(float f)
{
	int prec = 0;
	while ((f-(int64_t)f) != 0.0f && prec <= 11) {
		f *= 10.0f;
		prec++;
	}
	return prec + 1;
}
