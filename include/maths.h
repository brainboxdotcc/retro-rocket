#pragma once

#include "kernel.h"

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
