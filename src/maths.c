#include <kernel.h>

#define MAX_CIRCLE_ANGLE      512
#define HALF_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/2)
#define QUARTER_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/4)
#define MASK_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE - 1)
#define PI 3.14159265358979323846

/* Split ln(2) to minimise reduction error: ln2 = ln2_hi + ln2_lo */
static const double ln2_hi = 0x1.62e42fefa39efp-1;      /*  0.693147180559945309417232121458176568  */
static const double ln2_lo = 0x1.abc9e3b39803fp-56;     /*  1.9082149292705877000e-10               */

/* Thresholds to avoid spurious overflow/underflow in exp */
static const double exp_overflow_lim  = 709.782712893383973096;   /* ~= log(DBL_MAX)  */
static const double exp_underflow_lim = -745.13321910194110842;   /* ~= log(DBL_MIN) - guard */

/**
 * @brief SIN/COS lookup table, precalculated using:
 * 
 *  for (i = 0 ; i < MAX_CIRCLE_ANGLE ; i++) {
 *  	lut[i] = sin((double)i * PI / HALF_MAX_CIRCLE_ANGLE);
 *  }
 */
const float lut[] = {
	0.000000,	0.0122715,	0.0245412,	0.0368072,	0.0490677,	0.0613207,	0.0735646,	0.0857973,
	0.0980171,	0.110222,	0.122411,	0.134581,	0.14673,	0.158858,	0.170962,	0.18304,
	0.19509,	0.207111,	0.219101,	0.231058,	0.24298,	0.254866,	0.266713,	0.27852,
	0.290285,	0.302006,	0.313682,	0.32531,	0.33689,	0.348419,	0.359895,	0.371317,
	0.382683,	0.393992,	0.405241,	0.41643,	0.427555,	0.438616,	0.449611,	0.460539,
	0.471397,	0.482184,	0.492898,	0.503538,	0.514103,	0.52459,	0.534998,	0.545325,
	0.55557,	0.565732,	0.575808,	0.585798,	0.595699,	0.605511,	0.615232,	0.62486,
	0.634393,	0.643832,	0.653173,	0.662416,	0.671559,	0.680601,	0.689541,	0.698376,
	0.707107,	0.715731,	0.724247,	0.732654,	0.740951,	0.749136,	0.757209,	0.765167,
	0.77301,	0.780737,	0.788346,	0.795837,	0.803208,	0.810457,	0.817585,	0.824589,
	0.83147,	0.838225,	0.844854,	0.851355,	0.857729,	0.863973,	0.870087,	0.87607,
	0.881921,	0.88764,	0.893224,	0.898674,	0.903989,	0.909168,	0.91421,	0.919114,
	0.92388,	0.928506,	0.932993,	0.937339,	0.941544,	0.945607,	0.949528,	0.953306,
	0.95694,	0.960431,	0.963776,	0.966976,	0.970031,	0.97294,	0.975702,	0.978317,
	0.980785,	0.983105,	0.985278,	0.987301,	0.989177,	0.990903,	0.99248,	0.993907,
	0.995185,	0.996313,	0.99729,	0.998118,	0.998795,	0.999322,	0.999699,	0.999925,
	1.00000,	0.999925,	0.999699,	0.999322,	0.998795,	0.998118,	0.99729,	0.996313,
	0.995185,	0.993907,	0.99248,	0.990903,	0.989177,	0.987301,	0.985278,	0.983105,
	0.980785,	0.978317,	0.975702,	0.97294,	0.970031,	0.966976,	0.963776,	0.960431,
	0.95694,	0.953306,	0.949528,	0.945607,	0.941544,	0.937339,	0.932993,	0.928506,
	0.92388,	0.919114,	0.91421,	0.909168,	0.903989,	0.898674,	0.893224,	0.88764,
	0.881921,	0.87607,	0.870087,	0.863973,	0.857729,	0.851355,	0.844854,	0.838225,
	0.83147,	0.824589,	0.817585,	0.810457,	0.803208,	0.795837,	0.788346,	0.780737,
	0.77301,	0.765167,	0.757209,	0.749136,	0.740951,	0.732654,	0.724247,	0.715731,
	0.707107,	0.698376,	0.689541,	0.680601,	0.671559,	0.662416,	0.653173,	0.643831,
	0.634393,	0.624859,	0.615232,	0.605511,	0.595699,	0.585798,	0.575808,	0.565732,
	0.55557,	0.545325,	0.534998,	0.52459,	0.514103,	0.503538,	0.492898,	0.482184,
	0.471397,	0.460539,	0.449611,	0.438616,	0.427555,	0.416429,	0.405241,	0.393992,
	0.382683,	0.371317,	0.359895,	0.348419,	0.33689,	0.32531,	0.313682,	0.302006,
	0.290285,	0.27852,	0.266713,	0.254866,	0.24298,	0.231058,	0.219101,	0.207111,
	0.19509,	0.18304,	0.170962,	0.158858,	0.14673,	0.134581,	0.122411,	0.110222,
	0.0980171,	0.0857972,	0.0735645,	0.0613207,	0.0490676,	0.0368071,	0.0245411,	0.0122715,
	-8.74228e-08,	-0.0122716,	-0.0245413,	-0.0368073,	-0.0490678,	-0.0613208,	-0.0735647,	-0.0857974,
	-0.0980172,	-0.110222,	-0.122411,	-0.134581,	-0.146731,	-0.158858,	-0.170962,	-0.18304,
	-0.19509,	-0.207111,	-0.219101,	-0.231058,	-0.24298,	-0.254866,	-0.266713,	-0.27852,
	-0.290285,	-0.302006,	-0.313682,	-0.32531,	-0.33689,	-0.348419,	-0.359895,	-0.371317,
	-0.382684,	-0.393992,	-0.405241,	-0.41643,	-0.427555,	-0.438616,	-0.449611,	-0.460539,
	-0.471397,	-0.482184,	-0.492898,	-0.503538,	-0.514103,	-0.52459,	-0.534998,	-0.545325,
	-0.55557,	-0.565732,	-0.575808,	-0.585798,	-0.595699,	-0.605511,	-0.615232,	-0.62486,
	-0.634393,	-0.643832,	-0.653173,	-0.662416,	-0.671559,	-0.680601,	-0.689541,	-0.698376,
	-0.707107,	-0.715731,	-0.724247,	-0.732654,	-0.740951,	-0.749136,	-0.757209,	-0.765167,
	-0.773011,	-0.780737,	-0.788346,	-0.795837,	-0.803208,	-0.810457,	-0.817585,	-0.824589,
	-0.83147,	-0.838225,	-0.844854,	-0.851355,	-0.857729,	-0.863973,	-0.870087,	-0.87607,
	-0.881921,	-0.88764,	-0.893224,	-0.898674,	-0.903989,	-0.909168,	-0.91421,	-0.919114,
	-0.92388,	-0.928506,	-0.932993,	-0.937339,	-0.941544,	-0.945607,	-0.949528,	-0.953306,
	-0.95694,	-0.960431,	-0.963776,	-0.966977,	-0.970031,	-0.97294,	-0.975702,	-0.978317,
	-0.980785,	-0.983106,	-0.985278,	-0.987301,	-0.989177,	-0.990903,	-0.99248,	-0.993907,
	-0.995185,	-0.996313,	-0.99729,	-0.998118,	-0.998795,	-0.999322,	-0.999699,	-0.999925,
	-1.00000,	-0.999925,	-0.999699,	-0.999322,	-0.998795,	-0.998118,	-0.99729,	-0.996313,
	-0.995185,	-0.993907,	-0.99248,	-0.990903,	-0.989177,	-0.987301,	-0.985278,	-0.983105,
	-0.980785,	-0.978317,	-0.975702,	-0.97294,	-0.970031,	-0.966976,	-0.963776,	-0.960431,
	-0.95694,	-0.953306,	-0.949528,	-0.945607,	-0.941544,	-0.937339,	-0.932993,	-0.928506,
	-0.92388,	-0.919114,	-0.91421,	-0.909168,	-0.903989,	-0.898674,	-0.893224,	-0.88764,
	-0.881921,	-0.87607,	-0.870087,	-0.863973,	-0.857729,	-0.851355,	-0.844853,	-0.838225,
	-0.83147,	-0.824589,	-0.817585,	-0.810457,	-0.803207,	-0.795837,	-0.788346,	-0.780737,
	-0.77301,	-0.765167,	-0.757209,	-0.749136,	-0.740951,	-0.732654,	-0.724247,	-0.715731,
	-0.707107,	-0.698376,	-0.68954,	-0.680601,	-0.671559,	-0.662416,	-0.653173,	-0.643831,
	-0.634393,	-0.624859,	-0.615231,	-0.605511,	-0.595699,	-0.585798,	-0.575808,	-0.565732,
	-0.55557,	-0.545325,	-0.534997,	-0.52459,	-0.514103,	-0.503538,	-0.492898,	-0.482184,
	-0.471397,	-0.460539,	-0.449611,	-0.438616,	-0.427555,	-0.416429,	-0.405241,	-0.393992,
	-0.382683,	-0.371317,	-0.359895,	-0.348419,	-0.33689,	-0.32531,	-0.313682,	-0.302006,
	-0.290285,	-0.27852,	-0.266713,	-0.254865,	-0.24298,	-0.231058,	-0.219101,	-0.207111,
	-0.19509,	-0.18304,	-0.170962,	-0.158858,	-0.14673,	-0.134581,	-0.122411,	-0.110222,
	-0.098017,	-0.0857971,	-0.0735644,	-0.0613206,	-0.0490675,	-0.036807,	-0.0245411,	-0.0122714,
};

/* Polynomial for exp on reduced interval r ∈ [-ln2/2, +ln2/2].
   Minimax coefficients (degree 6), evaluated with Estrin/Horner hybrid. */
static inline double exp_poly(double r)
{
	/* Coefficients for e^r ≈ 1 + r + r^2*(c2 + r*(c3 + r*(c4 + r*(c5 + r*c6)))) */
	const double c2 = 0.5;
	const double c3 = 1.6666666666666665741e-1;  /* 1/6 */
	const double c4 = 4.1666666666666592922e-2;  /* 1/24 */
	const double c5 = 8.3333333333291961946e-3;  /* 1/120 */
	const double c6 = 1.3888888888873056412e-3;  /* 1/720 */

	double r2 = r * r;
	double p = c6;
	p = c5 + r * p;
	p = c4 + r * p;
	p = c3 + r * p;
	p = c2 + r * p;

	return 1.0 + r + r2 * p;
}

double exp(double x)
{
	/* NaN and infinities */
	if (isnan(x)) {
		return x;
	}
	if (x > exp_overflow_lim) {
		/* Overflow to +inf */
		return INFINITY;
	}
	if (x < exp_underflow_lim) {
		/* Underflow to 0 */
		return 0.0;
	}

	/* Argument reduction: x = k*ln2 + r, with r in [-ln2/2, +ln2/2] */
	double k_real = nearbyint(x / ln2_hi);            /* k rounded to nearest to minimise r */
	int k = (int)k_real;

	/* Use split ln2 to keep r tiny and accurate */
	double r = (x - k_real * ln2_hi) - k_real * ln2_lo;

	/* Compute e^r via polynomial, then scale by 2^k */
	double er = exp_poly(r);
	return scalbn(er, k);
}

/* Polynomial for log on core interval using log1p-style form.
   For m in [sqrt(1/2), sqrt(2)], set u = (m-1)/(m+1), then:
   log(m) = 2*(u + u^3/3 + u^5/5 + ...). Use an odd polynomial up to u^9. */
static inline double log_core_from_m(double m)
{
	/* u in roughly [-0.1716, +0.1716]; series converges fast */
	double u = (m - 1.0) / (m + 1.0);
	double u2 = u * u;

	/* 2*(u + u^3/3 + u^5/5 + u^7/7 + u^9/9)
	   Group terms to reduce rounding. */
	double s1 = u + (u * u2) * (1.0 / 3.0);
	double s2 = (u * u2 * u2 * u2) * (1.0 / 5.0);
	double s3 = (u * u2 * u2 * u2 * u2 * u2) * (1.0 / 7.0);
	double s4 = (u * u2 * u2 * u2 * u2 * u2 * u2 * u2) * (1.0 / 9.0);

	return 2.0 * (s1 + s2 + s3 + s4);
}

double log(double x)
{
	/* Domain checks */
	if (isnan(x)) {
		return x;
	}
	if (x == 0.0) {
		/* log(0) = -inf, with divide-by-zero flag */
		return -INFINITY;
	}
	if (x < 0.0) {
		/* log(negative) = NaN */
		return NAN;
	}
	if (isinf(x)) {
		return INFINITY;
	}

	/* Decompose x = m * 2^k, with m in [0.5, 1) */
	int k = 0;
	double m = frexp(x, &k);  /* x = m * 2^k, m ∈ [0.5, 1) */

	/* Re-normalise m to lie in [sqrt(1/2), sqrt(2)] for faster convergence.
	   If m < sqrt(1/2), double m and decrement k; if m >= sqrt(2), halve and increment k.
	   Since frexp already gives [0.5,1), only the first adjustment is needed. */
	const double sqrt_half = 0x1.6a09e667f3bcdp-1; /* sqrt(1/2) */
	if (m < sqrt_half) {
		m *= 2.0;
		k -= 1;
	}

	/* log(x) = k*ln2 + log(m), and log(m) via stable core */
	double log_m = log_core_from_m(m);
	double result = k * ln2_hi + log_m;
	result += k * ln2_lo;  /* add low part to improve accuracy */

	return result;
}

double exp2(double x) {
	return exp(x * ln2_hi) * exp(x * ln2_lo);
}


double pow(double x, double y)
{
	if (y == 0.0) {
		return 1.0;
	}
	if (x == 0.0 && y > 0.0) {
		return 0.0;
	}
	return exp(y * log(x));
}

double factorial(int n)
{
	int f = 1;
	for (int i = n; i > 0; i--) {
		f *= i;
	}
	return f;
}

double sin(double rads)
{
	double f = rads * HALF_MAX_CIRCLE_ANGLE / PI;
	int64_t i = (int64_t)f;
	if (i < 0) {
		return lut[(-((-i)&MASK_MAX_CIRCLE_ANGLE)) + MAX_CIRCLE_ANGLE];
	} else {
		return lut[i&MASK_MAX_CIRCLE_ANGLE];
	}
}

double cos(double rads)
{
	double f = rads * HALF_MAX_CIRCLE_ANGLE / PI;
	int64_t i = (int64_t)f;
	if (i < 0) {
		return lut[((-i) + QUARTER_MAX_CIRCLE_ANGLE)&MASK_MAX_CIRCLE_ANGLE];
	} else {
		return lut[(i + QUARTER_MAX_CIRCLE_ANGLE)&MASK_MAX_CIRCLE_ANGLE];
	}
}

double tan(double rads)
{
	if (rads == 0) {
		return 0;
	}
	return sin(rads) / cos(rads);
}

double floor(double x)
{
	int64_t i = (int64_t)x;
	if (x < 0 && x != (double)i) {
		return (double)(i - 1);
	}
	return (double)i;
}

double fmod(double x, double y)
{
	if (y == 0.0) {
		return 0.0; // undefined, but safe return
	}
	double div = x / y;
	int64_t q = (int64_t)div;
	return x - (double)q * y;
}

double ceil(double x)
{
	int64_t i = (int64_t)x;
	if (x > 0 && x != (double)i) {
		return (double)(i + 1);
	}
	return (double)i;
}

double round(double x)
{
	int64_t i = (int64_t)x;
	double frac = x - (double)i;
	if (frac >= 0.5) {
		return (double)(i + 1);
	} else if (frac <= -0.5) {
		return (double)(i - 1);
	}
	return (double)i;
}

double atan2(double y, double x)
{
	if (x > 0) {
		return atan(y / x);
	} else if (x < 0 && y >= 0) {
		return atan(y / x) + PI;
	} else if (x < 0 && y < 0) {
		return atan(y / x) - PI;
	} else if (x == 0 && y > 0) {
		return PI / 2;
	} else if (x == 0 && y < 0) {
		return -PI / 2;
	}
	return 0.0; // undefined for x = 0 and y = 0
}

static inline double atan_core(double z)
{
	return z * (PI / 4) + 0.273 * z * (1 - fabs(z));
}

double atan(double x)
{
	if (x > 1.0) {
		return (PI / 2) - atan_core(1.0 / x);
	} else if (x < -1.0) {
		return -(PI / 2) - atan_core(1.0 / x);
	} else {
		return atan_core(x);
	}
}

double asin(double x)
{
	if (x > 1.0 || x < -1.0) {
		return rr_nan(); // Outside domain
	}
	return atan(x / sqrt(1.0 - x * x)); // asin(x) = atan(x / sqrt(1 - x²))
}

double acos(double x)
{
	if (x > 1.0 || x < -1.0) {
		return rr_nan(); // Outside domain
	}
	return (PI / 2) - asin(x); // acos(x) = π/2 - asin(x)
}

double deg(double radians)
{
	return radians * (180.0 / PI);
}

double rad(double degrees)
{
	return degrees * (PI / 180.0);
}

double ldexp(double x, int exp) {
	return __builtin_ldexp(x, exp);
}

float log2f(float x) {
	return log(x) * 1.4426950408889634f;
}
