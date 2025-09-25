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

double ldexp(double x, int exp);

float log2f(float x);

static inline uint8_t stdc_count_ones(uint32_t x) {
	return (uint8_t)__builtin_popcount(x);
}

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CLAMP(v, lo, hi) (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))

/* === Constants: INFINITY, NAN =========================================== */

#ifndef INFINITY
static const union { uint64_t u; double d; } __inf_union = { 0x7ff0000000000000ULL };
#define INFINITY (__inf_union.d)
#endif

#ifndef NAN
/* Quiet NaN (payload non-zero) */
static const union { uint64_t u; double d; } __nan_union = { 0x7ff8000000000001ULL };
#define NAN (__nan_union.d)
#endif

/* === isnan / isinf ======================================================= */

#ifndef isnan
static inline int isnan(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	return ((v.u & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) &&
	       ((v.u & 0x000fffffffffffffULL) != 0ULL);
}
#endif

#ifndef isinf
static inline int isinf(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	return (v.u & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL;
}
#endif

/* === fabs (used by your log) ============================================ */

#ifndef fabs
static inline double fabs(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	v.u &= 0x7fffffffffffffffULL;
	return v.d;
}
#endif

/* === nearbyint: round-to-nearest, ties-to-even =========================== */
/* Behaviour matches typical libc nearbyint in default rounding mode.      */
/* Valid for |x| < 2^52, which is plenty for exp/log range reduction.      */

#ifndef nearbyint
static inline double nearbyint(double x)
{
	const double two52 = 4503599627370496.0; /* 2^52 */
	if (x >= 0.0) {
		double y = x + two52;
		return y - two52;
	} else {
		double y = -x + two52;
		return -(y - two52);
	}
}
#endif

/* === scalbn: x * 2^n with correct special-case handling ================== */

#ifndef scalbn
static inline double scalbn(double x, int n)
{
	union { uint64_t u; double d; } v;
	v.d = x;

	uint64_t sign = v.u & 0x8000000000000000ULL;
	uint64_t mant = v.u & 0x000fffffffffffffULL;
	int _exp = (int)((v.u >> 52) & 0x7ff);

	/* NaN or Inf */
	if (_exp == 0x7ff) {
		return x;
	}

	/* Zero (+/-) */
	if (_exp == 0 && mant == 0) {
		return x;
	}

	/* Subnormal: normalise first */
	if (_exp == 0) {
		/* Scale by 2^54 to bring into normal range */
		const double two54  = 18014398509481984.0;   /* 2^54  */
		const double twom54 = 5.551115123125783e-17; /* 2^-54 */
		v.d = x * two54;
		_exp = (int)((v.u >> 52) & 0x7ff);            /* now normal or inf */
		if (_exp == 0x7ff) {
			return (sign ? -INFINITY : INFINITY);
		}
		_exp -= 54; /* account for the pre-scale */
		int e2 = _exp + n;
		if (e2 <= 0) {
			/* Back to subnormal or underflow to signed zero */
			if (e2 <= -54) {
				union { uint64_t u; double d; } z = { sign };
				return z.d;
			}
			v.u = sign | ((uint64_t)(e2 + 54) << 52) | (v.u & 0x000fffffffffffffULL);
			return v.d * twom54;
		}
		if (e2 >= 0x7ff) {
			return (sign ? -INFINITY : INFINITY);
		}
		v.u = sign | ((uint64_t)e2 << 52) | (v.u & 0x000fffffffffffffULL);
		return v.d;
	}

	/* Normal */
	int e = _exp + n;
	if (e <= 0) {
		/* Becomes subnormal or underflows to signed zero */
		if (e <= -54) {
			union { uint64_t u; double d; } z = { sign };
			return z.d;
		}
		/* Make exponent 1 (minimal normal) then scale down to subnormal */
		v.u = sign | (1ULL << 52) | mant;
		/* Multiply by 2^(e-1), which is <= 2^-1 … 2^-53 range */
		int k = e - 1;
		/* Compute 2^k as a double via exponent field */
		union { uint64_t u; double d; } t = { (uint64_t)((1023 + k) & 0x7ff) << 52 };
		return v.d * t.d;
	}
	if (e >= 0x7ff) {
		return (sign ? -INFINITY : INFINITY);
	}

	v.u = sign | ((uint64_t)e << 52) | mant;
	return v.d;
}
#endif

double exp2(double x);

#ifndef frexp
static inline double frexp(double x, int *exp)
{
	union { uint64_t u; double d; } v;
	v.d = x;

	uint64_t sign = v.u & 0x8000000000000000ULL;
	uint64_t mant = v.u & 0x000fffffffffffffULL;
	int iexp = (int)((v.u >> 52) & 0x7ff);

	if (iexp == 0) {
		/* Zero or subnormal */
		if (mant == 0) {
			*exp = 0;
			return 0.0; /* preserves sign of zero automatically */
		}

		/* Normalise subnormal: shift left until leading 1 appears */
		int shift = __builtin_clzll(mant) - (64 - 53);
		mant <<= shift;
		iexp = 1 - shift;
		v.u = sign | ((uint64_t)(1022) << 52) | (mant & 0x000fffffffffffffULL);
		*exp = iexp - 1022;
		return v.d;
	}

	if (iexp == 0x7ff) {
		/* Inf or NaN: return as-is, exp indeterminate */
		*exp = 0;
		return x;
	}

	/* Normal finite number: adjust exponent so mantissa ∈ [0.5, 1) */
	*exp = iexp - 1022;
	v.u = sign | (0x3feULL << 52) | mant; /* set exponent to 1022 (bias-1) */
	return v.d;
}
#endif

