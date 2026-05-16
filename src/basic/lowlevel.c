/**
 * @file basic/lowlevel.c
 * @brief BASIC low level (assembly/machine) functions
 */
#include <kernel.h>
#include <cpuid.h>

/**
 * @brief CPUID instruction result
 *
 * This structure holds the result of a CPUID instruction, which provides
 * detailed information about the CPU, such as supported features and capabilities.
 */
typedef struct cpuid_result {
	unsigned int eax; ///< The EAX register value from the CPUID instruction
	unsigned int ebx; ///< The EBX register value from the CPUID instruction
	unsigned int ecx; ///< The ECX register value from the CPUID instruction
	unsigned int edx; ///< The EDX register value from the CPUID instruction
} cpuid_result_t;

void write_cpuid(int leaf, cpuid_result_t* res)
{
	__cpuid(leaf, res->eax, res->ebx, res->ecx, res->edx);
}

void write_cpuidex(int leaf, int subleaf, cpuid_result_t* res)
{
	__cpuid_count(leaf, subleaf, res->eax, res->ebx, res->ecx, res->edx);
}

int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg, cpuid_result_t* res)
{
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
	memory_grants_add(&ctx->memory_grants, ctx->allocator, p, intval);
	return (int64_t)p;
}

int64_t basic_memrealloc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t handle = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t new_size = intval;
	PARAMS_END("MEMREALLOC", 0);
	void* p = buddy_realloc(ctx->allocator, handle, new_size);
	if (!p) {
		tokenizer_error_print(ctx, "Out of memory");
		return 0;
	}
	memory_grants_remove(&ctx->memory_grants, ctx->allocator, handle);
	memory_grants_add(&ctx->memory_grants, ctx->allocator, p, new_size);
	return (int64_t)p;
}

void memmove_statement(struct basic_ctx* ctx)
{
	accept_or_return(MEMMOVE, ctx);
	int64_t source = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t dest = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (!address_valid_read(source, size) || !address_valid_write(dest, size)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx or &%016lx of size &%016lx", source, size, dest, size);
		return;
	}
	if (size && is_restricted_len(ctx, "MEMORY", 6) && (!memory_grants_contains(&ctx->memory_grants, source, size) || !memory_grants_contains(&ctx->memory_grants, dest, size))) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx or &%016lx of size &%016lx", source, size, dest, size);
		return;
	}
	memmove(dest, source, size);
}

void memcopy_statement(struct basic_ctx* ctx)
{
	accept_or_return(MEMCOPY, ctx);
	int64_t source = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t dest = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (!address_valid_read(source, size) || !address_valid_write(dest, size)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx or &%016lx of size &%016lx", source, size, dest, size);
		return;
	}
	if (size && is_restricted_len(ctx, "MEMORY", 6) && (!memory_grants_contains(&ctx->memory_grants, source, size) || !memory_grants_contains(&ctx->memory_grants, dest, size))) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx or &%016lx of size &%016lx", source, size, dest, size);
		return;
	}
	memcpy(dest, source, size);
}

void memset_statement(struct basic_ctx* ctx)
{
	accept_or_return(MEMSET, ctx);
	int64_t dest = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (value < 0 || value > 255) {
		tokenizer_error_printf(ctx, "MEMSET: Invalid byte value %ld", value);
		return;
	}
	if (!address_valid_write(dest, size)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx", dest, size);
		return;
	}
	if (size && is_restricted_len(ctx, "MEMORY", 6) && !memory_grants_contains(&ctx->memory_grants, dest, size)) {
		tokenizer_error_printf(ctx, "Bad address &%016lx", dest);
		return;
	}
	memset(dest, (uint8_t)value, size);
}

int64_t basic_memfind(struct basic_ctx* ctx)
{
        PARAMS_START;
        PARAMS_GET_ITEM(BIP_INT);
        int64_t start = intval;
        PARAMS_GET_ITEM(BIP_INT);
        int64_t size = intval;
        PARAMS_GET_ITEM(BIP_INT);
        int64_t value = intval;

        PARAMS_END("MEMFIND", 0);

        if (value < 0 || value > 255) {
                tokenizer_error_printf(ctx, "MEMFIND: Invalid byte value %ld", value);
                return 0;
        }

        if (!address_valid_read(start, size)) {
                tokenizer_error_printf(ctx, "Bad Address at &%016lx of size &%016lx", start, size);
                return 0;
        }
	if (size && is_restricted_len(ctx, "MEMORY", 6) && !memory_grants_contains(&ctx->memory_grants, start, size)) {
		tokenizer_error_printf(ctx, "Bad address &%016lx", start);
		return 0;
	}


        return (int64_t)memchr((void*)start, (uint8_t)value, size);
}

char* basic_cpugetbrand(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool trim = intval;
	PARAMS_END("CPUGETBRAND$", "");

	const char *bufferp = cpu_caps.brand;

	if (trim) {
		while (*bufferp == ' ') {
			bufferp++;
		}
	}

	return (char *)gc_strdup(ctx, bufferp);
}

char* basic_cpugetvendor(struct basic_ctx* ctx)
{
	return (char *)gc_strdup(ctx, cpu_caps.vendor);
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
		return (char*)gc_strdup(ctx, "");
	}
	char result[16] = {0};
	(*(int64_t*) result) = target;
	result[length] = '\0';
	return (char*)gc_strdup(ctx, result);
}

int64_t basic_cpuid(struct basic_ctx* ctx)
{
	cpuid_result_t res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t leaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t subleaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t reg = intval;
	PARAMS_END("CPUID", -1);
	if (subleaf != -1) {
		write_cpuidex(leaf, subleaf, &res);
	} else {
		write_cpuid(leaf, &res);
	}
	return get_cpuid_reg(ctx, reg, &res);
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
	if (!memory_grants_valid_base(&ctx->memory_grants, ptr)) {
		tokenizer_error_printf(ctx, "Bad address &%016lx", ptr);
		return;
	}
	buddy_free(ctx->allocator, ptr);
	memory_grants_remove(&ctx->memory_grants, ctx->allocator, ptr);
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
