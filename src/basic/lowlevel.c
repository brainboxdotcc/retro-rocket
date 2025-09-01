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
	return inb(port);
}

int64_t basic_inportw(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_END("INPORTW", -1);
	return inw(port);
}

int64_t basic_inportd(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_END("INPORTD", -1);
	return inl(port);
}

void outport_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORT, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFF;
	accept_or_return(NEWLINE, ctx);
	outb(port, value);
}

void outportw_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORTW, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFFFF;
	accept_or_return(NEWLINE, ctx);
	outw(port, value);
}

void outportd_statement(struct basic_ctx* ctx) {
	accept_or_return(OUTPORTD, ctx);
	int64_t port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx) & 0xFFFFFFFF;
	accept_or_return(NEWLINE, ctx);
	outl(port, value);
}

