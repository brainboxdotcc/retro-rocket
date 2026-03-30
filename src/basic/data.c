#include <kernel.h>
#include "basic/unified_expression.h"

/**
 * @brief Append a value to data_store.
 */
int data_store_append(struct basic_ctx* ctx, struct data_store* ds, up_value value)
{
	up_value* new_values;

	new_values = buddy_realloc(ctx->allocator, ds->values, (ds->length + 1) * sizeof(up_value));
	if (!new_values) {
		return -1;
	}

	ds->values = new_values;
	ds->values[ds->length] = value;
	ds->length++;

	return 0;
}

/**
 * @brief Append an offset to data_sets.
 */
int data_sets_append(struct basic_ctx* ctx, struct data_sets* ds, int64_t offset)
{
	int64_t* new_offsets;

	new_offsets = buddy_realloc(ctx->allocator, ds->offsets, (ds->length + 1) * sizeof(int64_t));
	if (!new_offsets) {
		return -1;
	}

	ds->offsets = new_offsets;
	ds->offsets[ds->length] = offset;
	ds->length++;

	return 0;
}

void add_dataset_here(struct basic_ctx* ctx, char* search)
{
	/* Trim leading whitespace */
	while (*search && isspace((unsigned char)*search)) {
		++search;
	}

	/* Extract variable name (up to whitespace or end) */
	char* name = search;
	while (*search && !isspace((unsigned char)*search)) {
		++search;
	}

	if (*search) {
		*search = '\0';
	}

	if (!valid_int_var(name)) {
		tokenizer_error_printf(ctx, "Invalid integer variable for DATASET '%s'", name);
		return;
	}

	int64_t offset = ctx->datastore.length;
	data_sets_append(ctx, &ctx->datasets, offset);
	basic_set_int_variable(name, offset, ctx, false, false);
}

void extract_data_from_line(struct basic_ctx* ctx, char* search)
{
	while (*search) {

		/* Skip leading whitespace */
		while (*search && isspace(*search)) {
			++search;
		}

		if (!*search) {
			break;
		}

		if (*search == '"') {
			++search; /* start of string */
			char* data_str = search;

			while (*search && *search != '"') {
				++search;
			}

			if (*search == '"') {
				*search = '\0';
				++search;
			}

			const char* duplicated = buddy_strdup(ctx->allocator, data_str);
			if (!duplicated) {
				tokenizer_error_print(ctx, "Out of memory storing DATA");
				return;
			}
			data_store_append(ctx, &ctx->datastore, up_make_str(duplicated));
		} else {
			char* start = search;

			/* consume until comma or end */
			while (*search && *search != ',') {
				++search;
			}

			char saved = *search;
			*search = '\0';

			/* detect real vs int */
			int is_real = 0;
			for (char* p = start; *p; ++p) {
				if (*p == '.') {
					is_real = 1;
					break;
				}
			}

			if (is_real) {
				double v = strtod(start, NULL);
				data_store_append(ctx, &ctx->datastore, up_make_real(v));
			} else {
				int64_t v = atoll(start, 10);
				data_store_append(ctx, &ctx->datastore, up_make_int(v));
			}

			*search = saved;
		}

		/* Skip whitespace before comma */
		while (*search && isspace((unsigned char)*search)) {
			++search;
		}

		/* Skip comma */
		if (*search == ',') {
			++search;
		}
	}
}

void free_datastores(struct basic_ctx* ctx) {
	if (ctx->datasets.length > 0) {
		buddy_free(ctx->allocator, ctx->datasets.offsets);
	}
	if (ctx->datastore.length > 0) {
		buddy_free(ctx->allocator, ctx->datastore.values);
	}
	ctx->datasets.length = 0;
	ctx->datastore.length = 0;
	ctx->datasets.offsets = NULL;
	ctx->datastore.values = NULL;
}

void fill_datastores(struct basic_ctx* ctx) {
	ctx->datasets.length = 0;
	ctx->datastore.length = 0;
	ctx->datasets.offsets = NULL;
	ctx->datastore.values = NULL;
	char* program = (char*)ctx->ptr;

	while (true) {
		char const* linestart = program;
		while (*program != '\n' && *program != 0) {
			++program;
		}

		char const* lineend = program;

		char* linetext = buddy_malloc(ctx->allocator, lineend - linestart + 1);
		if (!linetext) {
			return;
		}
		strlcpy(linetext, linestart, lineend - linestart + 1);

		char* search = linetext;

		while (*search++ >= '0' && *search <= '9') {
			search++;
		}
		--search;

		while (*search++ == ' ');
		--search;

		if (!strncmp(search, "DATA ", 5)) {
			search += 5;
			extract_data_from_line(ctx, search);
		} else if (!strncmp(search, "DATASET ", 8)) {
			search += 8;
			add_dataset_here(ctx, search);
		}

		while (*program == '\n') {
			++program;
		}

		buddy_free(ctx->allocator, linetext);

		if (!*program) {
			break;
		}
	}
}

void data_statement(struct basic_ctx* ctx)
{
	accept_or_return(DATA, ctx);
	while (*ctx->ptr != '\n' && *ctx->ptr != 0) {
		++ctx->ptr;
	}
	ctx->nextptr = ctx->ptr + 1;
	tokenizer_next(ctx);
}

void dataset_statement(struct basic_ctx* ctx)
{
	accept_or_return(DATASET, ctx);
	while (*ctx->ptr != '\n' && *ctx->ptr != 0) {
		++ctx->ptr;
	}
	ctx->nextptr = ctx->ptr + 1;
	tokenizer_next(ctx);
}

void restore_statement(struct basic_ctx* ctx)
{
	accept_or_return(RESTORE, ctx);
	if (ctx->datastore.length == 0) {
		tokenizer_error_printf(ctx, "No DATA");
		return;
	}
	if (tokenizer_token(ctx) == NEWLINE) {
		ctx->data_offset = 0;
		return;
	}
	int64_t offset = expr(ctx);
	if (offset < 0 || offset >= (int64_t)ctx->datastore.length) {
		tokenizer_error_printf(ctx, "Invalid DATA offset; allowed range is [0..%lu]", ctx->datastore.length - 1);
		return;
	}
	ctx->data_offset = offset;
}

int64_t basic_dataread(struct basic_ctx* ctx)
{
	if (ctx->data_offset >= ctx->datastore.length) {
		tokenizer_error_print(ctx, "Out of DATA");
		return 0;
	}
	up_value v = ctx->datastore.values[ctx->data_offset++];
	if (v.kind != UP_INT) {
		tokenizer_error_print(ctx, "Expected integer DATA");
		return 0;
	}
	return v.v.i;
}

void basic_dataread_real(struct basic_ctx* ctx, double* rv)
{
	if (ctx->data_offset >= ctx->datastore.length) {
		tokenizer_error_print(ctx, "Out of DATA");
		return;
	}
	up_value v = ctx->datastore.values[ctx->data_offset++];
	if (v.kind == UP_REAL) {
		*rv = v.v.r;
		return;
	} else if (v.kind == UP_INT) {
		*rv = (double)v.v.i;
		return;
	}
	tokenizer_error_print(ctx, "Expected real DATA");
}

char* basic_dataread_string(struct basic_ctx* ctx)
{
	if (ctx->data_offset >= ctx->datastore.length) {
		tokenizer_error_print(ctx, "Out of DATA");
		return "";
	}
	up_value v = ctx->datastore.values[ctx->data_offset++];
	if (v.kind != UP_STR) {
		tokenizer_error_print(ctx, "Expected string DATA");
		return "";
	}
	return (char*)gc_strdup(ctx, v.v.s);
}
