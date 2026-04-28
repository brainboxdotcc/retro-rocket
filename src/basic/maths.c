/**
 * @file basic/maths.c
 * @brief BASIC maths functions
 */
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

void basic_minr(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m1 = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m2 = doubleval;
	PARAMS_END_VOID("MINR");
	*res = MIN(m1, m2);
}

void basic_maxr(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m1 = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m2 = doubleval;
	PARAMS_END_VOID("MAXR");
	*res = MAX(m1, m2);
}

int64_t basic_min(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m1 = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m2 = doubleval;
	PARAMS_END("MIN", 0);
	return MIN(m1, m2);
}

int64_t basic_max(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m1 = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double m2 = doubleval;
	PARAMS_END("MAX", 0);
	return MAX(m1, m2);
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

void basic_atan(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("ATAN");
	*res = atan(doubleval);
}

void basic_atan2(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double y = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double x = doubleval;
	PARAMS_END_VOID("ATAN2");
	*res = atan2(y, x);
}

void basic_ceil(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("CEIL");
	*res = ceil(doubleval);
}

void basic_round(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("ROUND");
	*res = round(doubleval);
}

void basic_fmod(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double x = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double y = doubleval;
	PARAMS_END_VOID("FMOD");
	if (y == 0.0) {
		tokenizer_error_print(ctx, "FMOD divide by zero");
		*res = 0.0;
		return;
	}
	*res = fmod(x, y);
}

int64_t basic_random(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t low = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t high = intval;
	PARAMS_END("RND", 0);
	int64_t out;
	mt_rand_range(low, high, &out);
	return out;
}

int64_t basic_secure_random(struct basic_ctx* ctx)
{
	int64_t out;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t low = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t high = intval;
	PARAMS_END("SECRND", 0);
	if (!csprng_range(low, high, &out)) {
		tokenizer_error_print(ctx, "Unable to generate secure random number, try again later");
		return 0;
	}
	return out;
}

char* basic_secure_random_string(struct basic_ctx* ctx)
{
	static const char* default_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t generate_len = intval;
	char* out_str = buddy_malloc(ctx->allocator, generate_len);
	if (generate_len < 1 || !out_str) {
		tokenizer_error_print(ctx, "Invalid length of secure random string to generate");
		return "";
	}
	PARAMS_GET_ITEM(BIP_STRING);
	const char* alphabet = strval;
	PARAMS_END("SECSTR$", "");
	if (!*alphabet) {
		alphabet = default_alphabet;
	}
	if (!csprng_string_from_alphabet(out_str, generate_len, alphabet, strlen(alphabet))) {
		buddy_free(ctx->allocator, out_str);
		tokenizer_error_print(ctx, "Unable to generate secure random string, try again later");
		return "";
	}
	char* out = (char*)gc_strdup(ctx, out_str);
	buddy_free(ctx->allocator, out_str);
	return out;
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

int64_t basic_atoi(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t radix = intval;
	PARAMS_END("ATOI", 0);
	int64_t res = 0;
	int err = 0;
	if ((err = do_atoi(&res, target, radix)) >= 0) {
		return res;
	}
	switch (-err) {
		case 1:
			tokenizer_error_print(ctx, "Invalid character");
			break;
		case 2:
			tokenizer_error_print(ctx, "Empty string");
			break;
		case 4:
			tokenizer_error_print(ctx, "Invalid radix");
			break;
	}
	return 0;
}

int64_t basic_shl(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t x = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t y = intval;
	PARAMS_END("SHL", 0);
	return x << y;
}

int64_t basic_shr(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t x = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t y = intval;
	PARAMS_END("SHR", 0);
	return x >> y;
}

void basic_asn(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("ASN");
	if (doubleval < -1.0 || doubleval > 1.0) {
		tokenizer_error_print(ctx, "ASN argument out of range (-1..1)");
		*res = 0.0;
		return;
	}
	*res = asin(doubleval);
}

void basic_acs(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("ACS");
	if (doubleval < -1.0 || doubleval > 1.0) {
		tokenizer_error_print(ctx, "ACS argument out of range (-1..1)");
		*res = 0.0;
		return;
	}
	*res = acos(doubleval);
}

void basic_exp(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("EXP");
	*res = exp(doubleval);
}

void basic_log(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("LOG");
	if (doubleval <= 0.0) {
		tokenizer_error_print(ctx, "LOG argument must be > 0");
		*res = 0.0;
		return;
	}
	*res = log(doubleval);
}

void basic_deg(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("DEG");
	*res = deg(doubleval);
}

void basic_rad(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("RAD");
	*res = rad(doubleval);
}

int64_t basic_sgn(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t val = intval;
	PARAMS_END("SGN", 0);

	if (val > 0) return 1;
	if (val < 0) return -1;
	return 0;
}

int64_t basic_int(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double val = doubleval;
	PARAMS_END("INT", 0);

	return (int64_t)val; // truncates toward zero
}

