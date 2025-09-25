/**
 * @file maths.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

/**
 * @brief Return a quiet NaN (Not a Number).
 *
 * @details Produces an IEEE-754 quiet NaN with a non-zero payload. Used when
 * a result is undefined (e.g., log of a non-positive value) or to propagate
 * invalid operations. Behaviour on signalling NaNs is implementation defined.
 *
 * @return Quiet NaN as a double.
 */
static inline double rr_nan(void) {
	union { uint64_t u; double d; } v = { 0x7ff8000000000000ULL }; // NaN
	return v.d;
}

/**
 * @brief Raise a base to a power.
 *
 * @details Computes \f$ \text{base}^{\text{exp}} \f$ for double arguments.
 * For negative bases with non-integer exponents the result is NaN in the reals.
 * Special values: \c pow(x,0)=1 for any finite x; \c pow(0,y)=0 for y>0.
 *
 * @param base Base value.
 * @param exp Exponent value.
 * @return Result of base to the power of exp.
 */
double pow(double base, double exp);

/**
 * @brief Compute factorial of an integer (n!).
 *
 * @details Defined for \c n >= 0. Grows rapidly and will overflow the range
 * of double for modest \c n (≈ 171). Runtime is \c O(n).
 *
 * @param n Non-negative integer.
 * @return n! as a double, or +INF on overflow.
 */
double factorial(int n);

/**
 * @brief Sine of an angle in radians.
 *
 * @details Uses an internal lookup/approximation; accuracy sufficient for
 * general graphics/audio. Input not range-reduced beyond typical wrap.
 *
 * @param rads Angle in radians.
 * @return sin(rads).
 */
double sin(double rads);

/**
 * @brief Cosine of an angle in radians.
 *
 * @details Uses an internal lookup/approximation; accuracy sufficient for
 * general graphics/audio. Input not range-reduced beyond typical wrap.
 *
 * @param rads Angle in radians.
 * @return cos(rads).
 */
double cos(double rads);

/**
 * @brief Tangent of an angle in radians.
 *
 * @details Implemented as sin(x)/cos(x). Undefined (±INF/NaN) near odd
 * multiples of \f$\pi/2\f$ where cosine approaches zero.
 *
 * @param rads Angle in radians.
 * @return tan(rads), NaN/INF near poles.
 */
double tan(double rads);

/**
 * @brief Square root.
 *
 * @details Domain: \c x >= 0. For negative \c x returns NaN.
 *
 * @param x Input value.
 * @return sqrt(x) for x>=0, otherwise NaN.
 */
double sqrt(double x);

/**
 * @brief Floor (round toward −infinity).
 *
 * @param x Input value.
 * @return Largest integer value \c <= x, as double.
 */
double floor(double x);

/**
 * @brief Floating-point remainder.
 *
 * @details Computes \c x - n*y where \c n is the integer quotient of \c x/y
 * truncated toward zero. Domain: \c y != 0.
 *
 * @param x Dividend.
 * @param y Divisor (non-zero).
 * @return Remainder of x divided by y.
 */
double fmod(double x, double y);

/**
 * @brief Ceiling (round toward +infinity).
 *
 * @param x Input value.
 * @return Smallest integer value \c >= x, as double.
 */
double ceil(double x);

/**
 * @brief Round to nearest integer.
 *
 * @details Values halfway between integers are rounded away from zero.
 *
 * @param x Input value.
 * @return Nearest integer, as double.
 */
double round(double x);

/**
 * @brief Two-argument arctangent.
 *
 * @details Returns the angle of the vector (x,y) in radians in \f$(-\pi,\pi]\f$,
 * using the signs of both arguments to select the correct quadrant.
 * Domain: (x,y) ≠ (0,0).
 *
 * @param y Numerator component.
 * @param x Denominator component.
 * @return Angle in radians, or 0 if both x and y are 0.
 */
double atan2(double y, double x);

/**
 * @brief Arctangent (inverse tangent).
 *
 * @details Returns an angle in radians within \f(-\pi/2, \pi/2\f).
 *
 * @param x Input value.
 * @return atan(x) in radians.
 */
double atan(double x);

/**
 * @brief Arcsine (inverse sine).
 *
 * @details Domain: \c x in [-1,1]. Returns NaN outside this range.
 *
 * @param x Input value.
 * @return asin(x) in radians, or NaN if |x|>1.
 */
double asin(double x);

/**
 * @brief Arccosine (inverse cosine).
 *
 * @details Domain: \c x in [-1,1]. Returns NaN outside this range.
 *
 * @param x Input value.
 * @return acos(x) in radians, or NaN if |x|>1.
 */
double acos(double x);

/**
 * @brief Natural exponential.
 *
 * @details Computes \f$e^x\f$. Large positive \c x may overflow to +INF;
 * large negative \c x underflow toward 0.
 *
 * @param x Exponent.
 * @return e raised to x.
 */
double exp(double x);

/**
 * @brief Natural logarithm.
 *
 * @details Domain: \c x > 0. Returns NaN for \c x <= 0. For subnormals
 * precision may degrade to a few ulp depending on platform.
 *
 * @param x Positive input.
 * @return ln(x), or NaN if x<=0.
 */
double log(double x);

/**
 * @brief Convert radians to degrees.
 *
 * @param radians Angle in radians.
 * @return Angle in degrees.
 */
double deg(double radians);

/**
 * @brief Convert degrees to radians.
 *
 * @param degrees Angle in degrees.
 * @return Angle in radians.
 */
double rad(double degrees);

/**
 * @brief Scale by power of two.
 *
 * @details Computes \c x * 2^exp with correct handling of subnormals,
 * zeros, infinities and NaNs. Useful for fast range scaling.
 *
 * @param x Input value.
 * @param exp Integer power of two.
 * @return x * 2^exp.
 */
double ldexp(double x, int exp);

/**
 * @brief Base-2 logarithm (float).
 *
 * @details Domain: \c x > 0. Returns NaN for \c x <= 0.
 * Single-precision result suitable for lightweight uses.
 *
 * @param x Positive input.
 * @return log2(x) as float, or NaN if x<=0.
 */
float log2f(float x);

/**
 * @brief Count set bits (population count) in a 32-bit word.
 *
 * @details Uses compiler builtin where available.
 *
 * @param x 32-bit input value.
 * @return Number of one bits in x (0..32).
 */
static inline uint8_t stdc_count_ones(uint32_t x) {
	return (uint8_t)__builtin_popcount(x);
}

/**
 * @brief Minimum of two values (macro).
 *
 * @warning Arguments may be evaluated more than once; avoid passing
 * expressions with side effects.
 */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Maximum of two values (macro).
 *
 * @warning Arguments may be evaluated more than once; avoid passing
 * expressions with side effects.
 */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp a value to a closed interval [lo, hi] (macro).
 *
 * @details Uses non-strict comparisons to avoid type-limits warnings with
 * unsigned types. Requires \c lo <= \c hi for meaningful results.
 *
 * @param v Value expression.
 * @param lo Lower bound.
 * @param hi Upper bound.
 * @return v limited to the range [lo, hi].
 */
#define CLAMP(v, lo, hi)  (((v) <= (lo)) ? (lo) : (((v) >= (hi)) ? (hi) : (v)))

/**
 * @brief Positive infinity constant for double.
 *
 * @details Provided when not available from a hosted <math.h>.
 */
static const union { uint64_t u; double d; } __inf_union = { 0x7ff0000000000000ULL };
#define INFINITY (__inf_union.d)

/**
 * @brief Quiet NaN constant for double.
 *
 * @details Provided when not available from a hosted <math.h>.
 */
static const union { uint64_t u; double d; } __nan_union = { 0x7ff8000000000001ULL };
#define NAN (__nan_union.d)

/**
 * @brief Test for NaN (Not a Number).
 *
 * @details True if \c x has all exponent bits set and a non-zero mantissa.
 * Works on IEEE-754 doubles.
 *
 * @param x Input value.
 * @return Non-zero if x is NaN, otherwise zero.
 */
static inline int isnan(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	return ((v.u & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) &&
	       ((v.u & 0x000fffffffffffffULL) != 0ULL);
}

/**
 * @brief Test for infinity.
 *
 * @details True if \c x is +INF or −INF (exponent all ones, mantissa zero).
 *
 * @param x Input value.
 * @return Non-zero if x is infinite, otherwise zero.
 */
static inline int isinf(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	return (v.u & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL;
}

/**
 * @brief Absolute value for double.
 *
 * @param x Input value.
 * @return |x|.
 */
static inline double fabs(double x)
{
	union { uint64_t u; double d; } v;
	v.d = x;
	v.u &= 0x7fffffffffffffffULL;
	return v.d;
}

/**
 * @brief Round to nearest integer, ties-to-even (no fenv).
 *
 * @details Accurate for |x| < 2^52. Used in range reduction for transcendental functions.
 *
 * @param x Input value.
 * @return x rounded to the nearest integer, as double.
 */
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

/**
 * @brief Scale by power of two with full IEEE-754 edge-case handling.
 *
 * @details Computes \c x * 2^n even for subnormals and preserves signed zero,
 * infinities and NaNs. Avoids reliance on hosted libm.
 *
 * @param x Input value.
 * @param n Power of two.
 * @return x scaled by 2^n.
 */
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

/**
 * @brief Base-2 exponential.
 *
 * @details Computes \c 2^x. Large positive \c x may overflow to +INF; large
 * negative \c x underflow toward 0. Provided to avoid reliance on hosted libm.
 *
 * @param x Exponent.
 * @return 2 raised to x.
 */
double exp2(double x);

/**
 * @brief Decompose a double into mantissa and exponent.
 *
 * @details Returns a mantissa in [0.5,1) and stores the exponent such that
 * \c x == mantissa * 2^*exp. For zero returns 0 and sets *exp = 0.
 * For NaN/INF returns the input and sets *exp = 0.
 *
 * @param x Input value.
 * @param exp Output pointer to receive the exponent.
 * @return Mantissa in [0.5,1) with the sign of x, or special cases as noted.
 */
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
