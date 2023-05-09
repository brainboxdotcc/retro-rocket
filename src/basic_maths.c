#include <kernel.h>

int64_t basic_abs(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("ABS",0);
	return labs(intval);
}

void basic_sin(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("SIN");
	*res = sin(doubleval);
}

void basic_cos(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("COS");
	*res = cos(doubleval);
}

void basic_realval(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END_VOID("REALVAL");
	atof(strval, res);
	return;
}

void basic_tan(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("TAN");
	*res = tan(doubleval);
}

void basic_pow(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double base = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("POW");
	*res = pow(base, doubleval);
}

void basic_sqrt(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double v = doubleval;
	PARAMS_END_VOID("SQRT");
	*res = sqrt(v);
}

int64_t basic_random(struct basic_ctx* ctx)
{
	int64_t low, high;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	low = intval;
	PARAMS_GET_ITEM(BIP_INT);
	high = intval;
	PARAMS_END("RND", 0);
	return (mt_rand() % (high - low + 1)) + low;
}

int64_t basic_asc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("ASC", 0);
	return (unsigned char)*strval;
}

int64_t basic_val(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("VAL", 0);
	return atoll(strval, 10);
}

int64_t basic_hexval(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("HEXVAL", 0);
	return atoll(strval, 16);
}

int64_t basic_octval(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("OCTVAL", 0);
	return atoll(strval, 8);
}

