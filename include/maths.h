/**
 * @file maths.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

static inline double rr_nan(void) {
	union { uint64_t u; double d; } v = { 0x7ff8000000000000ULL }; // NaN
	return v.d;
}

/**
 * @brief Raise base to the power of exp
 * 
 * @param base Base number
 * @param exp Exponent
 * @return double base ** exp
 */
double pow(double base, double exp);

/**
 * @brief Calculate factorial (n!)
 * @note This function is not recursive, but will
 * take O(n) time to calculate based on the integer
 * value of n.
 * 
 * @param n number to factorialise
 * @return double n!
 */
double factorial(int n);

/**
 * @brief Calculate sine of rads, using a lookup table
 * 
 * @param rads radians
 * @return double sine of radians
 */
double sin(double rads);

/**
 * @brief Calculate cosine of rads, using a lookup table
 * 
 * @param rads radians
 * @return double cosine of radians
 */
double cos(double rads);

/**
 * @brief Calculate tangent of rads, using sin x / cos x
 * 
 * @param rads radians
 * @return double tangent of radians
 */
double tan(double rads);

/**
 * @brief Calculate square root of x
 *
 * @param x the number whose square root is to be calculated
 * @return double calculated square root of `x`
 */
double sqrt(double x);

/**
 * @brief Returns the absolute value of a floating point number.
 *
 * @param x The input value.
 * @return The absolute value of x (|x|).
 */
double fabs(double x);

/**
 * @brief Rounds a floating point number down to the nearest integer value.
 *
 * @param x The input value.
 * @return The largest integer value less than or equal to x, as a double.
 */
double floor(double x);

/**
 * @brief Computes the floating-point remainder of x / y.
 *
 * The result is x - n * y, where n is the integer quotient of x / y,
 * truncated toward zero.
 *
 * @param x Dividend.
 * @param y Divisor (must not be zero).
 * @return The remainder of x divided by y.
 */
double fmod(double x, double y);

/**
 * @brief Rounds a floating-point value up to the nearest integer.
 *
 * @param x Input value.
 * @return The smallest integer value not less than x.
 */
double ceil(double x);

/**
 * @brief Rounds a floating-point value to the nearest integer.
 *
 * Values halfway between two integers are rounded away from zero.
 *
 * @param x Input value.
 * @return The nearest integer as a double.
 */
double round(double x);

/**
 * @brief Computes the arc tangent of y/x using the signs of both arguments
 * to determine the correct quadrant of the result.
 *
 * @param y y-coordinate (numerator).
 * @param x x-coordinate (denominator).
 * @return The angle in radians between -PI and PI.
 */
double atan2(double y, double x);

/**
 * @brief Computes the arc tangent of x (inverse tangent).
 *
 * The result is in radians, between -PI/2 and PI/2.
 *
 * @param x The input value.
 * @return The arc tangent of x.
 */
double atan(double x);

/**
 * @brief Returns the arc sine (inverse sine) of x.
 * @param x Input value (must be between -1 and 1).
 * @return The arcsin of x in radians.
 */
double asin(double x);

/**
 * @brief Returns the arc cosine (inverse cosine) of x.
 * @param x Input value (must be between -1 and 1).
 * @return The arccos of x in radians.
 */
double acos(double x);

/**
 * @brief Calculates e raised to the power of x.
 * @param x Exponent.
 * @return e^x.
 */
double exp(double x);

/**
 * @brief Computes the natural logarithm (ln) of x.
 * @param x Input value (must be > 0).
 * @return ln(x).
 */
double log(double x);

/**
 * @brief Converts radians to degrees.
 * @param radians Value in radians.
 * @return Value in degrees.
 */
double deg(double radians);

/**
 * @brief Converts degrees to radians.
 * @param degrees Value in degrees.
 * @return Value in radians.
 */
double rad(double degrees);

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
