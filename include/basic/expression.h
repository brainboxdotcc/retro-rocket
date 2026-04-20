#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

#define STRING_ESCAPE_BYTE 0xFF
#define STRING_ESCAPED_NUL 0x01
#define STRING_ESCAPED_ESC 0x02

#define STRING_ESCAPED_SIZE(_buf, _cur_len, _out_len) \
do { \
	size_t _ses_i; \
	(_out_len) = (size_t)(_cur_len); \
	for (_ses_i = 0; _ses_i < (size_t)(_cur_len); ++_ses_i) { \
		uint8_t _ses_b = (uint8_t)(_buf)[_ses_i]; \
		if (_ses_b == 0 || _ses_b == STRING_ESCAPE_BYTE) { \
			++(_out_len); \
		} \
	} \
} while (0)

#define STRING_ESCAPE_INPLACE(_buf, _cur_len, _max_len) \
do { \
	size_t _sei_old_len = (size_t)(_cur_len); \
	size_t _sei_new_len; \
	size_t _sei_src; \
	size_t _sei_dst; \
	STRING_ESCAPED_SIZE((_buf), _sei_old_len, _sei_new_len); \
	if (_sei_new_len + 1 > (size_t)(_max_len)) { \
		break; \
	} \
	if (_sei_new_len == _sei_old_len) { \
		(_buf)[_sei_old_len] = 0; \
		break; \
	} \
	_sei_src = _sei_old_len; \
	_sei_dst = _sei_new_len; \
	while (_sei_src > 0) { \
		uint8_t _sei_b = (uint8_t)(_buf)[_sei_src - 1]; \
		if (_sei_b == 0) { \
			(_buf)[_sei_dst - 2] = (char)STRING_ESCAPE_BYTE; \
			(_buf)[_sei_dst - 1] = (char)STRING_ESCAPED_NUL; \
			_sei_dst -= 2; \
		} else if (_sei_b == STRING_ESCAPE_BYTE) { \
			(_buf)[_sei_dst - 2] = (char)STRING_ESCAPE_BYTE; \
			(_buf)[_sei_dst - 1] = (char)STRING_ESCAPED_ESC; \
			_sei_dst -= 2; \
		} else { \
			(_buf)[_sei_dst - 1] = (char)_sei_b; \
			_sei_dst -= 1; \
		} \
		_sei_src -= 1; \
	} \
	(_buf)[_sei_new_len] = 0; \
} while (0)

#define STRING_UNESCAPED_SIZE(_buf, _cur_len, _out_len) \
do { \
	size_t _sus_i = 0; \
	(_out_len) = 0; \
	while (_sus_i < (size_t)(_cur_len)) { \
		if ((uint8_t)(_buf)[_sus_i] == STRING_ESCAPE_BYTE && _sus_i + 1 < (size_t)(_cur_len)) { \
			_sus_i += 2; \
		} else { \
			_sus_i += 1; \
		} \
		++(_out_len); \
	} \
} while (0)

#define STRING_UNESCAPE_INPLACE(_buf, _cur_len) \
do { \
	size_t _sui_src = 0; \
	size_t _sui_dst = 0; \
	while (_sui_src < (size_t)(_cur_len)) { \
		uint8_t _sui_b = (uint8_t)(_buf)[_sui_src]; \
		if (_sui_b == STRING_ESCAPE_BYTE && _sui_src + 1 < (size_t)(_cur_len)) { \
			uint8_t _sui_e = (uint8_t)(_buf)[_sui_src + 1]; \
			if (_sui_e == STRING_ESCAPED_NUL) { \
				(_buf)[_sui_dst] = 0; \
			} else if (_sui_e == STRING_ESCAPED_ESC) { \
				(_buf)[_sui_dst] = (char)STRING_ESCAPE_BYTE; \
			} else { \
				(_buf)[_sui_dst] = (_buf)[_sui_src + 1]; \
			} \
			_sui_src += 2; \
		} else { \
			(_buf)[_sui_dst] = (_buf)[_sui_src]; \
			_sui_src += 1; \
		} \
		_sui_dst += 1; \
	} \
	(_buf)[_sui_dst] = 0; \
	(_cur_len) = (int64_t)_sui_dst; \
} while (0)

/**
 * @brief Evaluates a full integer expression.
 *
 * This function parses and evaluates an integer expression from the BASIC program
 * based on the provided context. It supports all standard integer operations.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the evaluated integer expression as a 64-bit signed integer.
 */
int64_t expr(struct basic_ctx* ctx);

/**
 * @brief Evaluates a relational integer expression.
 *
 * This function evaluates relational expressions (e.g., <, >, ==) in the
 * context of integer operations. It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the relational expression as a 64-bit signed integer (`1` for true, `0` for false).
 */
int64_t relation(struct basic_ctx* ctx);


/**
 * @brief Evaluates a full real (double) expression.
 *
 * This function parses and evaluates a real (double) expression from the BASIC program
 * based on the provided context. It supports floating-point arithmetic operations.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @param res A pointer to a double where the result will be stored.
 */
void double_expr(struct basic_ctx* ctx, double* res);

/**
 * @brief Evaluates a relational real (double) expression.
 *
 * This function evaluates relational expressions (e.g., <, >, ==) for real numbers.
 * It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @param res A pointer to a double where the result will be stored.
 */
void double_relation(struct basic_ctx* ctx, double* res);


/**
 * @brief Evaluates a full string expression.
 *
 * This function parses and evaluates a string expression from the BASIC program
 * based on the provided context. It supports string concatenation and manipulation.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return A pointer to the resulting string from the evaluated expression.
 */
const char* str_expr(struct basic_ctx* ctx);

/**
 * @brief Evaluates a relational string expression.
 *
 * This function evaluates relational expressions (e.g., ==, <, >) for strings.
 * It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the relational string expression as a 64-bit signed integer (`1` for true, `0` for false).
 */
int64_t str_relation(struct basic_ctx* ctx);
