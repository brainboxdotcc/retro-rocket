/**
 * @file basic/bignum.h
 * @brief BASIC big number functions
 */
#pragma once

#include <kernel.h>

/**
 * @brief Add two big integers
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigadd(struct basic_ctx* ctx);

/**
 * @brief Subtract two big integers
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigsub(struct basic_ctx* ctx);

/**
 * @brief Multiply two big integers
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigmul(struct basic_ctx* ctx);

/**
 * @brief Divide two big integers (quotient)
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigdiv(struct basic_ctx* ctx);

/**
 * @brief Modulo of two big integers
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigmod(struct basic_ctx* ctx);

/**
 * @brief Compare two big integers
 * @param ctx BASIC execution context
 * @return -1, 0, or 1
 */
int64_t basic_bigcmp(struct basic_ctx* ctx);

/**
 * @brief Absolute value of big integer
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigabs(struct basic_ctx* ctx);

/**
 * @brief Negate big integer
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigneg(struct basic_ctx* ctx);

/**
 * @brief Left shift big integer
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigshl(struct basic_ctx* ctx);

/**
 * @brief Right shift big integer
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigshr(struct basic_ctx* ctx);

/**
 * @brief Greatest common divisor
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_biggcd(struct basic_ctx* ctx);

/**
 * @brief Modular exponentiation
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigmodpow(struct basic_ctx* ctx);

/**
 * @brief Modular inverse
 * @param ctx BASIC execution context
 * @return Result as string
 */
char* basic_bigmodinv(struct basic_ctx* ctx);
