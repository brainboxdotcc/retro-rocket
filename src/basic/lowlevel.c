/**
 * @file basic/lowlevel.c
 * @brief BASIC low level (assembly/machine) functions
 */
#include <kernel.h>
#include <cpuid.h>

void write_cpuid(struct basic_ctx* ctx, int leaf)
{
	__cpuid(
		leaf,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
}

void write_cpuidex(struct basic_ctx* ctx, int leaf, int subleaf)
{
	__cpuid_count(
		leaf,
		subleaf,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
}

int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg)
{
	cpuid_result_t* res = &ctx->last_cpuid_result;
	switch (reg) {
	case 0:
		return res->eax;
	case 1:
		return res->ebx;
	case 2:
		return res->ecx;
	case 3:
		return res->edx;
	}
	tokenizer_error_print(ctx, "Invalid register");
	return 0;
}

int64_t basic_legacy_cpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t leaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t subleaf = intval;
	PARAMS_END("LEGACYCPUID", -1);
	if (subleaf != -1) {
		write_cpuidex(ctx, leaf, subleaf);
		return 1;
	}
	write_cpuid(ctx, leaf);
	return 0;
}

int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("LEGACYGETLASTCPUID", -1);
	return get_cpuid_reg(ctx, intval);
}

int64_t basic_memalloc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("MEMALLOC", 0);
	void* p = buddy_malloc(ctx->allocator, intval);
	if (!p) {
		tokenizer_error_print(ctx, "Out of memory");
		return 0;
	}
	return (int64_t)p;
}

char* basic_cpugetbrand(struct basic_ctx* ctx)
{

	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool trim = intval;
	PARAMS_END("CPUGETBRAND$", "");
	char buffer[50] = {0};
	unsigned int* tmp = (unsigned int*) buffer;
	__cpuid(
		0x80000002,
		tmp[0],
		tmp[1],
		tmp[2],
		tmp[3]
	);
	__cpuid(
		0x80000003,
		tmp[4],
		tmp[5],
		tmp[6],
		tmp[7]
	);
	__cpuid(
		0x80000004,
		tmp[8],
		tmp[9],
		tmp[10],
		tmp[11]
	);
	buffer[48] = 0;
	char* bufferp = (char*) buffer;
	if (trim) {
		while (*bufferp == ' ') {
			++bufferp;
		}
	}
	return gc_strdup(ctx, bufferp);
}

char* basic_cpugetvendor(struct basic_ctx* ctx)
{
	__cpuid(
		0,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
	char buffer[13];
	((unsigned int*) buffer)[0] = ctx->last_cpuid_result.ebx;
	((unsigned int*) buffer)[1] = ctx->last_cpuid_result.edx;
	((unsigned int*) buffer)[2] = ctx->last_cpuid_result.ecx;
	buffer[12] = '\0';
	return gc_strdup(ctx, buffer);
}

char* basic_intoasc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t target = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t length = intval;
	PARAMS_END("INTOASC$", "");
	if (length < 0 || length > 8) {
		tokenizer_error_print(ctx, "Invalid length");
		return gc_strdup(ctx, "");
	}
	char result[16] = {0};
	(*(int64_t*) result) = target;
	result[length] = '\0';
	return gc_strdup(ctx, result);
}

int64_t basic_cpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t leaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t subleaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t reg = intval;
	PARAMS_END("CPUID", -1);
	if (subleaf != -1) {
		write_cpuidex(ctx, leaf, subleaf);
	} else {
		write_cpuid(ctx, leaf);
	}
	return get_cpuid_reg(ctx, reg);
}

int64_t basic_inport(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_END("INPORT", -1);
	return inb(port & 0xFFFF);
}

int64_t basic_inportw(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_END("INPORTW", -1);
	return inw(port & 0xFFFF);
}

int64_t basic_inportd(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_END("INPORTD", -1);
	return inl(port & 0xFFFF);
}

void outport_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORT, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFF;
	accept_or_return(NEWLINE, ctx);
	outb(port & 0xFFFF, value);
}

void outportw_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORTW, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFFFF;
	accept_or_return(NEWLINE, ctx);
	outw(port & 0xFFFF, value);
}

void memrelease_statement(struct basic_ctx* ctx) {
	accept_or_return(MEMRELEASE, ctx);
	int64_t ptr = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	buddy_free(ctx->allocator, ptr);
}

void outportd_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORTD, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFFFFFFFF;
	accept_or_return(NEWLINE, ctx);
	outl(port & 0xFFFF, value);
}

int64_t basic_bitor(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITOR", -1);
	return op1 | op2;
}

int64_t basic_bitand(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITAND", -1);
	return op1 & op2;
}

int64_t basic_bitnot(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_END("BITNOT", -1);
	return ~op1;
}

int64_t basic_biteor(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITEOR", -1);
	return op1 ^ op2;
}

int64_t basic_bitnand(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITNAND", -1);
	return ~(op1 & op2);
}

int64_t basic_bitnor(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITNOR", -1);
	return ~(op1 | op2);
}

int64_t basic_bitxnor(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op1 = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t op2 = intval;
	PARAMS_END("BITXNOR", -1);
	return ~(op1 ^ op2);
}

int64_t basic_bitshl(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t val = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t n = intval;
	PARAMS_END("BITSHL", -1);

	uint64_t u = (uint64_t) val;
	if (n < 0) n = 0;
	if (n > 63) n = 63;

	u = u << (unsigned)n;
	return (int64_t) u;
}

int64_t basic_bitshr(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t val = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t n = intval;
	PARAMS_END("BITSHR", -1);

	uint64_t u = (uint64_t) val; /* logical right shift */
	if (n < 0) n = 0;
	if (n > 63) n = 63;

	u = u >> (unsigned)n;
	return (int64_t) u;
}

int64_t basic_bitrol(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t val = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t n = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_END("BITROL", -1);

	uint64_t w = (width <= 0 || width > 64) ? 64u : (uint64_t) width;
	uint64_t mask = (w == 64u) ? ~0ULL : ((1ULL << w) - 1ULL);

	uint64_t k = (uint64_t) ((n % (int64_t)w + (int64_t)w) % (int64_t)w);
	uint64_t x = (uint64_t) val & mask;

	uint64_t out = ((x << k) | (x >> (w - k))) & mask;
	return (int64_t) out;
}

int64_t basic_bitror(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t val = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t n = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_END("BITROR", -1);

	uint64_t w = (width <= 0 || width > 64) ? 64u : (uint64_t) width;
	uint64_t mask = (w == 64u) ? ~0ULL : ((1ULL << w) - 1ULL);

	uint64_t k = (uint64_t) ((n % (int64_t)w + (int64_t)w) % (int64_t)w);
	uint64_t x = (uint64_t) val & mask;

	uint64_t out = ((x >> k) | (x << (w - k))) & mask;
	return (int64_t) out;
}
