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

static int64_t mul[] = {
        1,              // 0 decimals
        10,             // 1 decimal
        100,            // 2 decimals
        1000,           // 3 decimals
        10000,          // 4 decimals
        100000,         // 5 decimals
        1000000,        // 6 decimals
        10000000,       // 7 decimals
        100000000,      // 8 decimals
        1000000000,     // 9 decimals
        10000000000,    // 10 decimals
};

static inline uint8_t float_determine_decimal_places(double f)
{
        int prec = 0;
        while ((f-(int64_t)f) != 0.0 || f < 1) {
                f *= 10.0;
                prec++;
        }
        return prec + 1;
}

/**
 * @brief Convert a floating point number to char* string,
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
 * @param precision Precision of decimal part
 * @return char* pointer to value in buffer 
 */
static inline char * float_to_string(double x, char *p, int64_t len, uint8_t precision) {

	bool neg = x < 0.0;
	char buffer[64], buffer_part[64];
	char* index = buffer, *start = p, *decimal_pos = NULL;
	uint8_t decimals = float_determine_decimal_places(x);
	// 10 digits precision is the most this function supports
	decimals = decimals > 10 ? 10 : decimals;
	precision = precision > 10 ? 10 : precision;
	// Convert the double into an int64 copy of the whole part,
	// And a copy of the entire number multiplied up to a whole number.
	int64_t whole = labs((int64_t)x);
	int64_t integer_part = labs((int64_t)(x * (double)mul[decimals]));
	sprintf(buffer, "%llu", whole);
	sprintf(buffer_part, "%llu", integer_part);
	int64_t whole_len = strlen(buffer);
	int64_t integer_len = strlen(buffer_part);
	bool move_decimal = false;
	if (whole == 0) {
                move_decimal = true;
        }
	index = buffer_part;
	if (neg) *(p++) = '-';
	if (move_decimal) {
		*(p++) = '0';
		decimal_pos = p;
		*(p++) = '.';
		for (int64_t n = 0; n < (decimals - integer_len > 0 ? decimals - integer_len : 0); ++n) {
			*(p++) = '0';
		}
		for (int64_t n = 0; *index && n < len - 1; ++index, ++n) {
			*(p++) = *index;
		}
	} else {
		for (int64_t n = 0; n < integer_len && n < len - 1; ++index, ++n) {
			*(p++) = *index;
			if (n == whole_len - 1) {
				decimal_pos = p;
				*(p++) = '.';
			}
		}
	}
	*p = 0;
	// Remove any trailing 0's
	while (p != start && *(--p) == '0') {
		*p = 0;
	}
	// Remove trailing "."
	if (p != start && *p == '.') {
		decimal_pos = NULL;
		*p = 0;
	}
	// Now we can set precision by chopping it from the '.' or appending zeroes as neccessary
	// Precision display operates purely on the string representation.
	if (precision > 0) {
		if (decimal_pos == NULL) {
			// No decimal part is in the number, add one with trailing zeroes
			*(p++) = '.';
			for (int j = 0; j < precision; ++j) {
				*(p++) = '0';
			}
			*p = 0;
		} else {
			// There is an existing decimal part, find its length
			char* dec_ptr = decimal_pos + 1;
			uint8_t number_decimals = 0;
			while (*dec_ptr) {
				number_decimals++;
				dec_ptr++;
			}
			if (number_decimals > precision) {
				// more decimal than precision, cut it off
				*(decimal_pos + 1 + precision) = 0;
			} else {
				// Less decomals than precision, add more
				while (number_decimals < precision) {
					*dec_ptr = '0';
					dec_ptr++;
					number_decimals++;
				}
				*dec_ptr = 0;
			}
		}
	}
	return start;
}

