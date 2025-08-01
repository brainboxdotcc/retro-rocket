#include <kernel.h>

/**
 * @brief Powers of ten for multiplying a double to convert to
 * a string. We count the approximate number of decimal places
 * first using double_determine_decimal_places() which returns
 * a number between 0 and 10 corresponding to an entry in this
 * array. We then convert the float to a 64 bit signed integer,
 * and multiply it by this factor.
 */
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

/**
 * @brief Convert an unsigned 64-bit integer into a decimal string.
 * Returns a pointer into the provided buffer (writes backwards).
 */
static char* uint64_to_str(uint64_t val, char *buf, size_t buflen)
{
	char *end = buf + buflen - 1; // leave room for null
	*end = '\0';
	if (val == 0) {
		*--end = '0';
		return end;
	}
	while (val > 0 && end > buf) {
		*--end = '0' + (val % 10);
		val /= 10;
	}
	return end;
}

uint8_t double_determine_decimal_places(double f)
{
        int prec = 0;
        while (prec < 10 && ((f - (int64_t) f) != 0.0 || f < 1)) {
                f *= 10.0;
                ++prec;
        }
        return prec + 1;
}

char* double_to_string(double x, char *p, int64_t len, uint8_t precision)
{
	if (!p) {
		return NULL;
	}
	bool neg = x < 0.0;
	char buffer[64], buffer_part[64];
	char* index = buffer, *start = p, *decimal_pos = NULL;
	if (x == 0) {
		*p++ = '0';
		*p = 0;
	} else {
		uint8_t decimals = double_determine_decimal_places(x);
		// 10 digits precision is the most this function supports
		decimals = decimals > 10 ? 10 : decimals;
		precision = precision > 10 ? 10 : precision;
		// Convert the double into an int64 copy of the whole part,
		// And a copy of the entire number multiplied up to a whole number.
		int64_t whole = labs((int64_t)x);
		// We use the mul[] array rather than repeated multiplication,
		// because it is faster and does not cause a loss of accuracy.
		int64_t integer_part = labs((int64_t)(x * (double)mul[decimals]));

		// replaced snprintf with uint64_to_str
		char *whole_str = uint64_to_str((uint64_t)whole, buffer, sizeof(buffer));
		char *part_str  = uint64_to_str((uint64_t)integer_part, buffer_part, sizeof(buffer_part));

		int64_t whole_len = strlen(whole_str);
		int64_t integer_len = strlen(part_str);
		bool move_decimal = !whole;
		index = part_str;
		if (neg) *p++ = '-';
		if (move_decimal) {
			*p++ = '0';
			decimal_pos = p;
			*p++ = '.';
			for (int64_t n = 0; n < (decimals - integer_len > 0 ? decimals - integer_len : 0); ++n) {
				*p++ = '0';
			}
			for (int64_t n = 0; *index && n < len - 1; ++index, ++n) {
				*p++ = *index;
			}
		} else {
			for (int64_t n = 0; n < integer_len && n < len - 1; ++index, ++n) {
				*p++ = *index;
				if (n == whole_len - 1) {
					decimal_pos = p;
					*p++ = '.';
				}
			}
		}
		*p = 0;
		// Remove any trailing 0's
		while (p != start && *--p == '0') {
			*p = 0;
		}
		// Remove trailing "."
		if (p != start && *p == '.') {
			decimal_pos = NULL;
			*p = 0;
		}
	}
	// Now we can set precision by chopping it from the '.' or appending zeroes as neccessary
	// Precision display operates purely on the string representation.
	if (precision > 0) {
		if (decimal_pos == NULL) {
			// No decimal part is in the number, add one with trailing zeroes
			*p++ = '.';
			for (int j = 0; j < precision; ++j) {
				*p++ = '0';
			}
			*p = 0;
		} else {
			// There is an existing decimal part, find its length
			char* dec_ptr = decimal_pos + 1;
			uint8_t number_decimals = 0;
			while (*dec_ptr) {
				++number_decimals;
				++dec_ptr;
			}
			if (number_decimals > precision) {
				// more decimal than precision, cut it off
				*(decimal_pos + 1 + precision) = 0;
			} else {
				// Less decomals than precision, add more
				while (number_decimals < precision) {
					*dec_ptr++ = '0';
					++number_decimals;
				}
				*dec_ptr = 0;
			}
		}
	}
	return start;
}
