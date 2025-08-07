#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Computes the sine of a number.
 *
 * This function calculates the sine of the given input (in radians) and stores the result
 * in the provided `res` pointer. It is a wrapper around the standard sine function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the sine operation.
 */
void basic_sin(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the cosine of a number.
 *
 * This function calculates the cosine of the given input (in radians) and stores the result
 * in the provided `res` pointer. It is a wrapper around the standard cosine function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the cosine operation.
 */
void basic_cos(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the tangent of a number.
 *
 * This function calculates the tangent of the given input (in radians) and stores the result
 * in the provided `res` pointer. It is a wrapper around the standard tangent function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the tangent operation.
 */
void basic_tan(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the power of a number.
 *
 * This function calculates the result of raising the first parameter to the power of the second
 * parameter, and stores the result in the provided `res` pointer. It is a wrapper around the
 * standard power function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the power operation.
 */
void basic_pow(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the square root of a number.
 *
 * This function calculates the square root of the given input and stores the result
 * in the provided `res` pointer. It is a wrapper around the standard square root function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the square root operation.
 */
void basic_sqrt(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the arctangent of a number.
 *
 * This function calculates the arctangent (inverse tangent) of the given input (in radians) and
 * stores the result in the provided `res` pointer. It is a wrapper around the standard arctangent
 * function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the arctangent operation.
 */
void basic_atan(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the arctangent of two numbers.
 *
 * This function calculates the arctangent of two numbers (y/x) and stores the result in
 * the provided `res` pointer. It is a wrapper around the standard two-argument arctangent
 * function in the math library, useful for handling all quadrants of a coordinate system.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the arctangent operation.
 */
void basic_atan2(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the ceiling of a number.
 *
 * This function calculates the smallest integer greater than or equal to the given input and
 * stores the result in the provided `res` pointer. It is a wrapper around the standard ceiling
 * function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the ceiling operation.
 */
void basic_ceil(struct basic_ctx* ctx, double* res);

/**
 * @brief Rounds a number to the nearest integer.
 *
 * This function rounds the given input to the nearest integer and stores the result
 * in the provided `res` pointer. It is a wrapper around the standard round function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the rounding operation.
 */
void basic_round(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the remainder of a division.
 *
 * This function calculates the remainder when dividing the first number by the second,
 * and stores the result in the provided `res` pointer. It is a wrapper around the standard
 * floating-point modulus function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the modulus operation.
 */
void basic_fmod(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the arcsine of a number.
 *
 * This function calculates the arcsine (inverse sine) of the given input (in radians) and stores
 * the result in the provided `res` pointer. It is a wrapper around the standard arcsine function
 * in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the arcsine operation.
 */
void basic_asn(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the arccosine of a number.
 *
 * This function calculates the arccosine (inverse cosine) of the given input (in radians) and
 * stores the result in the provided `res` pointer. It is a wrapper around the standard arccosine
 * function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the arccosine operation.
 */
void basic_acs(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the exponential of a number.
 *
 * This function calculates the exponential of the given input, which is the constant `e` raised
 * to the power of the input, and stores the result in the provided `res` pointer. It is a wrapper
 * around the standard exponential function in the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the exponential operation.
 */
void basic_exp(struct basic_ctx* ctx, double* res);

/**
 * @brief Computes the natural logarithm of a number.
 *
 * This function calculates the natural logarithm (base `e`) of the given input and stores the
 * result in the provided `res` pointer. It is a wrapper around the standard logarithm function in
 * the math library.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the logarithmic operation.
 */
void basic_log(struct basic_ctx* ctx, double* res);

/**
 * @brief Converts a number from radians to degrees.
 *
 * This function converts the given input (in radians) to degrees and stores the result
 * in the provided `res` pointer. It is a simple calculation using the formula `degrees = radians * (180 / π)`.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the conversion.
 */
void basic_deg(struct basic_ctx* ctx, double* res);

/**
 * @brief Converts a number from degrees to radians.
 *
 * This function converts the given input (in degrees) to radians and stores the result
 * in the provided `res` pointer. It is a simple calculation using the formula `radians = degrees * (π / 180)`.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the conversion.
 */
void basic_rad(struct basic_ctx* ctx, double* res);

/**
 * @brief Converts a value to a real (double) value.
 *
 * This function converts the input to a real (double) value and stores the result
 * in the provided `res` pointer. It handles type conversion from integer or string to double.
 *
 * @param ctx The interpreter context.
 * @param res Pointer to the result of the conversion.
 */
void basic_realval(struct basic_ctx* ctx, double* res);
