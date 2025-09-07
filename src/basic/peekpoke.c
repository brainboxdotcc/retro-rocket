#include <kernel.h>
#include <stdint.h>
#include <stdbool.h>

extern volatile struct limine_memmap_request memory_map_request;

static inline bool span_overflows(uint64_t addr, uint64_t width) {
	if (width == 0) {
		return true;
	}
	uint64_t end = addr + width - 1;
	return end < addr;
}

static inline bool type_readable(uint64_t t) {
	return t == LIMINE_MEMMAP_USABLE
	       || t == LIMINE_MEMMAP_ACPI_NVS
	       || t == LIMINE_MEMMAP_ACPI_RECLAIMABLE
	       || t == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
	       || t == LIMINE_MEMMAP_FRAMEBUFFER
	       || t == LIMINE_MEMMAP_KERNEL_AND_MODULES;
}

static inline bool type_writable(uint64_t t) {
	return t == LIMINE_MEMMAP_USABLE
	       || t == LIMINE_MEMMAP_ACPI_RECLAIMABLE
	       || t == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
	       || t == LIMINE_MEMMAP_FRAMEBUFFER;
}

static inline bool address_valid_span(uint64_t addr, uint64_t width, bool for_write) {
	if (addr < 0x1000 || span_overflows(addr, width)) {
		return false;
	}

	const uint64_t end = addr + width - 1;
	uint64_t cursor = addr;

	const struct limine_memmap_response *resp = memory_map_request.response;
	if (!resp || resp->entry_count == 0 || !resp->entries) {
		return false;
	}

	while (cursor <= end) {
		const struct limine_memmap_entry *hit = NULL;

		for (uint64_t i = 0; i < resp->entry_count; ++i) {
			const struct limine_memmap_entry *e = resp->entries[i];
			const uint64_t e_base = e->base;
			const uint64_t e_end  = e->base + e->length - 1;
			if (cursor >= e_base && cursor <= e_end) {
				const bool ok = for_write ? type_writable(e->type) : type_readable(e->type);
				if (ok) {
					hit = e;
				}
				break;
			}
		}
		if (!hit) {
			return false;
		}

		const uint64_t hit_end = hit->base + hit->length - 1;
		if (hit_end >= end) {
			return true;
		}
		cursor = hit_end + 1;
	}
	return true;
}

static inline bool address_valid_read(uint64_t addr, int8_t width) {
	return address_valid_span(addr, width, false);
}

static inline bool address_valid_write(uint64_t addr, int8_t width) {
	return address_valid_span(addr, width, true);
}

/* ---- BASIC PEEK wrappers (error message unchanged) ---- */

int64_t basic_peek(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint64_t addr = (uint64_t)intval;
	PARAMS_END("PEEK", 0);

	if (!address_valid_read(addr, 1)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", addr);
		return 0;
	}
	uint8_t v = *(volatile uint8_t *)(uintptr_t)addr;
	return (int64_t)v;
}

int64_t basic_peekw(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint64_t addr = (uint64_t)intval;
	PARAMS_END("PEEKW", 0);

	if (!address_valid_read(addr, 2)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", addr);
		return 0;
	}
	uint16_t v = *(volatile uint16_t *)(uintptr_t)addr;
	return (int64_t)v;
}

int64_t basic_peekd(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint64_t addr = (uint64_t)intval;
	PARAMS_END("PEEKD", 0);

	if (!address_valid_read(addr, 4)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", addr);
		return 0;
	}
	uint32_t v = *(volatile uint32_t *)(uintptr_t)addr;
	return (int64_t)v;
}

int64_t basic_peekq(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint64_t addr = (uint64_t)intval;
	PARAMS_END("PEEKQ", 0);

	if (!address_valid_read(addr, 8)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", addr);
		return 0;
	}
	uint64_t v = *(volatile uint64_t *)(uintptr_t)addr;
	return (int64_t)v;
}

void poke_statement(struct basic_ctx* ctx) {
	accept_or_return(POKE, ctx);
	int64_t addr = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t val = expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (!address_valid_write((uint64_t)addr, 1)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", (uint64_t)addr);
		return;
	}
	*(volatile uint8_t *)(uintptr_t)addr = (uint8_t)val;
}

void pokew_statement(struct basic_ctx* ctx) {
	accept_or_return(POKEW, ctx);
	int64_t addr = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t val = expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (!address_valid_write((uint64_t)addr, 2)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", (uint64_t)addr);
		return;
	}
	*(volatile uint16_t *)(uintptr_t)addr = (uint16_t)val;
}

void poked_statement(struct basic_ctx* ctx) {
	accept_or_return(POKED, ctx);
	int64_t addr = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t val = expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (!address_valid_write((uint64_t)addr, 4)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", (uint64_t)addr);
		return;
	}
	*(volatile uint32_t *)(uintptr_t)addr = (uint32_t)val;
}

void pokeq_statement(struct basic_ctx* ctx) {
	accept_or_return(POKEQ, ctx);
	int64_t addr = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t val = expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (!address_valid_write((uint64_t)addr, 8)) {
		tokenizer_error_printf(ctx, "Bad Address at &%016lx", (uint64_t)addr);
		return;
	}
	*(volatile uint64_t *)(uintptr_t)addr = (uint64_t)val;
}

void modload_statement(struct basic_ctx* ctx) {
	accept_or_return(MODLOAD, ctx);
	const char* name = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (!load_module(name)) {
		tokenizer_error_printf(ctx, "Unable to load module '%s'", name);
		return;
	}
}

void modunload_statement(struct basic_ctx* ctx) {
	accept_or_return(MODUNLOAD, ctx);
	const char* name = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (!unload_module(name)) {
		tokenizer_error_printf(ctx, "Unable to unload module '%s'", name);
		return;
	}
}

