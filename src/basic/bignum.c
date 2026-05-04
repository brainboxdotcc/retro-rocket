#include <kernel.h>
#include <mbedtls/bignum.h>

typedef int (*basic_big_binary_func)(mbedtls_mpi* result, const mbedtls_mpi* a, const mbedtls_mpi* b);
typedef int (*basic_big_unary_func)(mbedtls_mpi* result, const mbedtls_mpi* a);

static void basic_big_error(struct basic_ctx* ctx, const char* name, int rc)
{
	if (rc == MBEDTLS_ERR_MPI_INVALID_CHARACTER) {
		tokenizer_error_printf(ctx, "%s invalid character", name);
		return;
	}

	if (rc == MBEDTLS_ERR_MPI_DIVISION_BY_ZERO) {
		tokenizer_error_printf(ctx, "%s divide by zero", name);
		return;
	}

	if (rc == MBEDTLS_ERR_MPI_NEGATIVE_VALUE) {
		tokenizer_error_printf(ctx, "%s negative value", name);
		return;
	}

	tokenizer_error_printf(ctx, "%s failed", name);
}

static int basic_big_read(struct basic_ctx* ctx, const char* name, mbedtls_mpi* value, const char* text)
{
	int rc = mbedtls_mpi_read_string(value, 10, text);
	if (rc != 0) {
		basic_big_error(ctx, name, rc);
		return false;
	}

	return true;
}

static char* basic_big_write(struct basic_ctx* ctx, const char* name, mbedtls_mpi* value)
{
	size_t size = 32;

	for (;;) {
		char* buffer = buddy_malloc(ctx->allocator, size);
		if (!buffer) {
			tokenizer_error_printf(ctx, "%s out of memory", name);
			return "";
		}

		size_t written = 0;
		int rc = mbedtls_mpi_write_string(value, 10, buffer, size, &written);
		if (rc == 0) {
			char* out = (char*)gc_strdup(ctx, buffer);
			buddy_free(ctx->allocator, buffer);
			return out;
		}

		buddy_free(ctx->allocator, buffer);

		if (rc != MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL) {
			basic_big_error(ctx, name, rc);
			return "";
		}

		size *= 2;
	}
}

static char* basic_big_binary(struct basic_ctx* ctx, const char* name, const char* a_text, const char* b_text, basic_big_binary_func func)
{
	mbedtls_mpi a;
	mbedtls_mpi b;
	mbedtls_mpi result;

	mbedtls_mpi_init(&a);
	mbedtls_mpi_init(&b);
	mbedtls_mpi_init(&result);

	char* out = "";

	if (basic_big_read(ctx, name, &a, a_text) && basic_big_read(ctx, name, &b, b_text)) {
		int rc = func(&result, &a, &b);
		if (rc == 0) {
			out = basic_big_write(ctx, name, &result);
		} else {
			basic_big_error(ctx, name, rc);
		}
	}

	mbedtls_mpi_free(&result);
	mbedtls_mpi_free(&b);
	mbedtls_mpi_free(&a);

	return out;
}

static char* basic_big_unary(struct basic_ctx* ctx, const char* name, const char* a_text, basic_big_unary_func func)
{
	mbedtls_mpi a;
	mbedtls_mpi result;

	mbedtls_mpi_init(&a);
	mbedtls_mpi_init(&result);

	char* out = "";

	if (basic_big_read(ctx, name, &a, a_text)) {
		int rc = func(&result, &a);
		if (rc == 0) {
			out = basic_big_write(ctx, name, &result);
		} else {
			basic_big_error(ctx, name, rc);
		}
	}

	mbedtls_mpi_free(&result);
	mbedtls_mpi_free(&a);

	return out;
}

static int basic_big_abs_func(mbedtls_mpi* result, const mbedtls_mpi* a)
{
	int rc = mbedtls_mpi_copy(result, a);
	if (rc != 0) {
		return rc;
	}

	result->s = 1;
	return 0;
}

static int basic_big_neg_func(mbedtls_mpi* result, const mbedtls_mpi* a)
{
	int rc = mbedtls_mpi_copy(result, a);
	if (rc != 0) {
		return rc;
	}

	if (mbedtls_mpi_cmp_int(result, 0) != 0) {
		result->s = -result->s;
	}

	return 0;
}

char* basic_bigadd(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b = strval;
	PARAMS_END("BIGADD$", "");
	return basic_big_binary(ctx, "BIGADD$", a, b, mbedtls_mpi_add_mpi);
}

char* basic_bigsub(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b = strval;
	PARAMS_END("BIGSUB$", "");
	return basic_big_binary(ctx, "BIGSUB$", a, b, mbedtls_mpi_sub_mpi);
}

char* basic_bigmul(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b = strval;
	PARAMS_END("BIGMUL$", "");
	return basic_big_binary(ctx, "BIGMUL$", a, b, mbedtls_mpi_mul_mpi);
}

char* basic_bigdiv(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a_text = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b_text = strval;
	PARAMS_END("BIGDIV$", "");

	mbedtls_mpi a;
	mbedtls_mpi b;
	mbedtls_mpi quotient;

	mbedtls_mpi_init(&a);
	mbedtls_mpi_init(&b);
	mbedtls_mpi_init(&quotient);

	char* out = "";

	if (basic_big_read(ctx, "BIGDIV$", &a, a_text) && basic_big_read(ctx, "BIGDIV$", &b, b_text)) {
		int rc = mbedtls_mpi_div_mpi(&quotient, NULL, &a, &b);
		if (rc == 0) {
			out = basic_big_write(ctx, "BIGDIV$", &quotient);
		} else {
			basic_big_error(ctx, "BIGDIV$", rc);
		}
	}

	mbedtls_mpi_free(&quotient);
	mbedtls_mpi_free(&b);
	mbedtls_mpi_free(&a);

	return out;
}

char* basic_bigmod(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b = strval;
	PARAMS_END("BIGMOD$", "");
	return basic_big_binary(ctx, "BIGMOD$", a, b, mbedtls_mpi_mod_mpi);
}

char* basic_biggcd(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b = strval;
	PARAMS_END("BIGGCD$", "");
	return basic_big_binary(ctx, "BIGGCD$", a, b, mbedtls_mpi_gcd);
}

char* basic_bigabs(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_END("BIGABS$", "");
	return basic_big_unary(ctx, "BIGABS$", a, basic_big_abs_func);
}

char* basic_bigneg(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_END("BIGNEG$", "");
	return basic_big_unary(ctx, "BIGNEG$", a, basic_big_neg_func);
}

char* basic_bigshl(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a_text = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t count = intval;
	PARAMS_END("BIGSHL$", "");

	if (count < 0) {
		tokenizer_error_print(ctx, "BIGSHL$ shift count must be >= 0");
		return "";
	}

	mbedtls_mpi a;

	mbedtls_mpi_init(&a);

	char* out = "";

	if (basic_big_read(ctx, "BIGSHL$", &a, a_text)) {
		int rc = mbedtls_mpi_shift_l(&a, count);
		if (rc == 0) {
			out = basic_big_write(ctx, "BIGSHL$", &a);
		} else {
			basic_big_error(ctx, "BIGSHL$", rc);
		}
	}

	mbedtls_mpi_free(&a);

	return out;
}

char* basic_bigshr(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a_text = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t count = intval;
	PARAMS_END("BIGSHR$", "");

	if (count < 0) {
		tokenizer_error_print(ctx, "BIGSHR$ shift count must be >= 0");
		return "";
	}

	mbedtls_mpi a;

	mbedtls_mpi_init(&a);

	char* out = "";

	if (basic_big_read(ctx, "BIGSHR$", &a, a_text)) {
		int rc = mbedtls_mpi_shift_r(&a, count);
		if (rc == 0) {
			out = basic_big_write(ctx, "BIGSHR$", &a);
		} else {
			basic_big_error(ctx, "BIGSHR$", rc);
		}
	}

	mbedtls_mpi_free(&a);

	return out;
}

char* basic_bigmodpow(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a_text = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* e_text = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* m_text = strval;
	PARAMS_END("BIGMODPOW$", "");

	mbedtls_mpi a;
	mbedtls_mpi e;
	mbedtls_mpi m;
	mbedtls_mpi result;

	mbedtls_mpi_init(&a);
	mbedtls_mpi_init(&e);
	mbedtls_mpi_init(&m);
	mbedtls_mpi_init(&result);

	char* out = "";

	if (basic_big_read(ctx, "BIGMODPOW$", &a, a_text) && basic_big_read(ctx, "BIGMODPOW$", &e, e_text) && basic_big_read(ctx, "BIGMODPOW$", &m, m_text)) {
		int rc = mbedtls_mpi_exp_mod(&result, &a, &e, &m, NULL);
		if (rc == 0) {
			out = basic_big_write(ctx, "BIGMODPOW$", &result);
		} else {
			basic_big_error(ctx, "BIGMODPOW$", rc);
		}
	}

	mbedtls_mpi_free(&result);
	mbedtls_mpi_free(&m);
	mbedtls_mpi_free(&e);
	mbedtls_mpi_free(&a);

	return out;
}

char* basic_bigmodinv(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* m = strval;
	PARAMS_END("BIGMODINV$", "");
	return basic_big_binary(ctx, "BIGMODINV$", a, m, mbedtls_mpi_inv_mod);
}

int64_t basic_bigcmp(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* a_text = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* b_text = strval;
	PARAMS_END("BIGCMP", 0);

	mbedtls_mpi a;
	mbedtls_mpi b;

	mbedtls_mpi_init(&a);
	mbedtls_mpi_init(&b);

	int64_t out = 0;

	if (basic_big_read(ctx, "BIGCMP", &a, a_text) && basic_big_read(ctx, "BIGCMP", &b, b_text)) {
		int cmp = mbedtls_mpi_cmp_mpi(&a, &b);
		if (cmp > 0) {
			out = 1;
		} else if (cmp < 0) {
			out = -1;
		}
	}

	mbedtls_mpi_free(&b);
	mbedtls_mpi_free(&a);

	return out;
}