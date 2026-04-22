#include <kernel.h>
#include <stdbool.h>
#include <basic/security.h>

extern struct basic_int_fn builtin_int[];
extern struct basic_str_fn builtin_str[];
extern struct basic_double_fn builtin_double[];

static uint64_t restriction_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const restriction_t *v = item;
	size_t len = v->length ? v->length : strlen(v->keyword);
	return hashmap_sip(v->keyword, len, seed0, seed1);
}

static int restriction_compare(const void *a, const void *b, void *udata) {
	const restriction_t *va = a;
	const restriction_t *vb = b;

	size_t la = va->length ? va->length : strlen(va->keyword);
	size_t lb = vb->length ? vb->length : strlen(vb->keyword);
	if (la != lb) {
		return (la < lb) ? -1 : 1;
	}
	if (la == 0) {
		return 0;
	}
	return strcmp(va->keyword, vb->keyword);
}

static void elfree_restriction(const void *item, void *udata)
{
	const restriction_t *e = item;
	struct buddy_allocator *a = udata;

	if (e->keyword) {
		buddy_free(a, e->keyword);
	}
}

bool basic_restrict_keyword_or_function_for_child(struct basic_ctx* ctx, const char* function_or_keyword) {
	if (!ctx->child_restrictions) {
		ctx->child_restrictions = hashmap_new_with_allocator(varmap_malloc, varmap_realloc, varmap_free, sizeof(restriction_t), 0, SEED0, SEED1, restriction_hash, restriction_compare, elfree_restriction, ctx->allocator);
		if (!ctx->child_restrictions) {
			tokenizer_error_print(ctx, "Out of memory creating restriction");
			return false;
		}
	}

	size_t len = strlen(function_or_keyword);
	const char* kw = buddy_strdup(ctx->allocator, function_or_keyword);
	if (!kw) {
		tokenizer_error_print(ctx, "Out of memory setting restriction");
		return false;
	}
	hashmap_set(ctx->child_restrictions, &(restriction_t){ .length = len, .keyword = kw });
	if (hashmap_oom(ctx->child_restrictions)) {
		tokenizer_error_print(ctx, "Out of memory setting restriction");
		return false;
	}

	return true;
}

bool is_restricted(struct basic_ctx* ctx, const char* function_or_keyword) {
	if (!ctx->active_restrictions) {
		return false;
	}
	return hashmap_get(ctx->active_restrictions, &(restriction_t){ .length = strlen(function_or_keyword), .keyword = function_or_keyword }) != NULL;
}

bool is_restricted_len(struct basic_ctx* ctx, const char* function_or_keyword, size_t length) {
	if (!ctx->active_restrictions) {
		return false;
	}
	return hashmap_get(ctx->active_restrictions, &(restriction_t){ .length = length, .keyword = function_or_keyword }) != NULL;
}
bool basic_derestrict_keyword_or_function_for_child(struct basic_ctx* ctx, const char* function_or_keyword) {
	if (!ctx->child_restrictions) {
		tokenizer_error_print(ctx, "Nothing to derestrict");
		return false;
	}

	restriction_t* found = hashmap_delete(ctx->child_restrictions, &(restriction_t){ .length = strlen(function_or_keyword), .keyword = function_or_keyword });
	if (!found) {
		tokenizer_error_print(ctx, "Keyword was not restricted");
		return false;
	}
	return true;
}

static bool restriction_iter(const void *item, void *udata)
{
	const restriction_t *entry = item;
	restriction_iter_ctx_t *iter = udata;
	const char* kw = buddy_strdup(iter->child->allocator, entry->keyword);
	if (!kw) {
		return false;
	}
	hashmap_set(iter->child->active_restrictions, &(restriction_t) { .length = entry->length, .keyword = kw });
	return !hashmap_oom(iter->child->active_restrictions);
}


bool basic_pass_restrictions_to_child(struct basic_ctx* parent, struct basic_ctx* child) {
	if (child->active_restrictions) {
		/* Already been called */
		return false;
	}
	if (!parent->child_restrictions && !parent->active_restrictions) {
		/* Nothing to restrict in child */
		return true;
	}
	child->active_restrictions = hashmap_new_with_allocator(varmap_malloc, varmap_realloc, varmap_free, sizeof(restriction_t), 0, SEED0, SEED1, restriction_hash, restriction_compare, elfree_restriction, child->allocator);
	if (!child->active_restrictions) {
		return false;
	}

	restriction_iter_ctx_t iter;
	memset(&iter, 0, sizeof(iter));
	iter.child = child;
	iter.parent = parent;

	if (parent->active_restrictions && !hashmap_scan(parent->active_restrictions, restriction_iter, &iter)) {
		return false;
	}

	if (parent->child_restrictions && !hashmap_scan(parent->child_restrictions, restriction_iter, &iter)) {
		return false;
	}

	return true;
}

static bool is_valid_restriction_name(const char *name) {
	GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
	GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)

	const size_t token_count = sizeof(token_names) / sizeof(*token_names);
	size_t len = strlen(name);

	for (size_t v = 0; v < token_count; ++v) {
		if (len == token_name_lengths[v] && !strcmp(name, token_names[v])) {
			return true;
		}
	}

	for (size_t v = 0; builtin_int[v].name; ++v) {
		if (!strcmp(name, builtin_int[v].name)) {
			return true;
		}
	}

	for (size_t v = 0; builtin_str[v].name; ++v) {
		if (!strcmp(name, builtin_str[v].name)) {
			return true;
		}
	}

	for (size_t v = 0; builtin_double[v].name; ++v) {
		if (!strcmp(name, builtin_double[v].name)) {
			return true;
		}
	}

	return false;
}

static void parse_restriction_list(struct basic_ctx *ctx, bool add)
{
	for (;;) {
		char name[MAX_STRINGLEN];

		while (*ctx->ptr == ' ' || *ctx->ptr == '\t') {
			++ctx->ptr;
		}

		if (*ctx->ptr == '\n' || *ctx->ptr == 0) {
			tokenizer_error_print(ctx, "Expected keyword or builtin function");
			return;
		}

		const char* start = ctx->ptr;

		while (*ctx->ptr != ',' && *ctx->ptr != '\n' && *ctx->ptr != 0) {
			++ctx->ptr;
		}

		const char* end = ctx->ptr;

		while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
			--end;
		}

		size_t len = end - start;
		if (len == 0) {
			tokenizer_error_print(ctx, "Expected keyword or builtin function");
			return;
		}

		strlcpy(name, start, len + 1);

		if (!is_valid_restriction_name(name)) {
			tokenizer_error_printf(ctx, "'%s' is not a valid keyword or builtin function", name);
			return;
		}

		if (add) {
			if (!basic_restrict_keyword_or_function_for_child(ctx, name)) {
				return;
			}
		} else {
			if (!basic_derestrict_keyword_or_function_for_child(ctx, name)) {
				return;
			}
		}

		if (*ctx->ptr == ',') {
			++ctx->ptr;
			continue;
		}

		break;
	}

	while (*ctx->ptr != '\n' && *ctx->ptr != 0) {
		++ctx->ptr;
	}

	ctx->nextptr = ctx->ptr + 1;
	tokenizer_next(ctx);
}

void restrict_statement(struct basic_ctx *ctx) {
	accept_or_return(RESTRICT, ctx);
	parse_restriction_list(ctx, true);
}

void derestrict_statement(struct basic_ctx *ctx) {
	accept_or_return(DERESTRICT, ctx);
	parse_restriction_list(ctx, false);
}