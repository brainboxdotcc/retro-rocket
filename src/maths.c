#include <kernel.h>

const int max_terms = 20;
static double* factorials;



double pow(double x, double y)
{
	if (y == 0) {
		return 1;
	} else if (x == 0 && y > 0) {
		return 0;
	}

	const double base = x;
	double value = base;
	double _pow = y;
	if (y < 0) {
		_pow = y * -1;
	}

	for (double i = 1; i < _pow; i++) {
		value *= base;
	}

	if (y < 0) {
		return 1 / value;
	}
	return value;
}

double factorial(int n)
{
	int f = 1;
	for (int i = n; i > 0; i--) {
		f *= i;
	}
	return f;
}

void init_maths()
{
	factorials = kmalloc(sizeof(double) * max_terms);
	for (int i = 1; i < max_terms + 1; i++) {
		factorials[i-1] = factorial(i);
	}
}

double sin(double rads)
{
	double result = rads;

	for (int cur_term=1; cur_term <= (max_terms / 2) - 1; cur_term++) {
		double cur_term_value = pow(rads, (cur_term * 2) + 1);
		cur_term_value /= factorials[cur_term * 2];
		if (cur_term & 0x01) {
			result -= cur_term_value;
		} else {
			result += cur_term_value;
		}
	}
	return result;
}

double cos(double rads)
{
	double result = 1.0;
	for (int cur_term = 1; cur_term <= (max_terms / 2) - 1; cur_term++) {
		double cur_term_value = pow(rads, (cur_term * 2));
		cur_term_value /= factorials[(cur_term * 2) - 1];
		if (cur_term & 0x01) {
			result -= cur_term_value;
		} else {
			result += cur_term_value;
		}
	}
	return result;
}

double tan(double rads)
{
	if (rads == 0) {
		return 0;
	}
	return sin(rads) / cos(rads);
}
