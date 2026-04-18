#include <map.h>
#include <kernel.h>

uint64_t int64_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const basic_map_handle_entry *e = item;
	return hashmap_sip(&e->id, sizeof(e->id), seed0, seed1);
}

int int64_compare(const void *a, const void *b, void *udata)
{
	const basic_map_handle_entry *ea = a;
	const basic_map_handle_entry *eb = b;

	if (ea->id < eb->id) {
		return -1;
	}

	if (ea->id > eb->id) {
		return 1;
	}

	return 0;
}

void elfree_map_handle_entry(const void *item, void *udata)
{
	const basic_map_handle_entry *e = item;

	if (e->map) {
		hashmap_free(e->map);
	}
}

uint64_t map_value_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const map_value_t *entry = item;
	size_t len = entry->name_length ? entry->name_length : strlen(entry->name);

	return hashmap_sip(entry->name, len, seed0, seed1);
}

int map_value_compare(const void *a, const void *b, void *udata)
{
	const map_value_t *ea = a;
	const map_value_t *eb = b;
	size_t la = ea->name_length ? ea->name_length : strlen(ea->name);
	size_t lb = eb->name_length ? eb->name_length : strlen(eb->name);

	if (la < lb) {
		return -1;
	}

	if (la > lb) {
		return 1;
	}

	return strcmp(ea->name, eb->name);
}

void elfree_map_value(const void *item, void *udata)
{
	const map_value_t *entry = item;
	struct basic_ctx *ctx = udata;

	if (entry->name) {
		buddy_free(ctx->allocator, (void *)entry->name);
	}

	if (entry->value.kind == UP_STR && entry->value.v.s) {
		buddy_free(ctx->allocator, (void *)entry->value.v.s);
	}
}

struct hashmap *basic_get_map_by_handle(struct basic_ctx *ctx, int64_t handle)
{
	basic_map_handle_entry *found;

	if (handle == 0) {
		return NULL;
	}

	found = hashmap_get(ctx->maps, &(basic_map_handle_entry){ .id = handle});
	if (!found || !found->map) {
		return NULL;
	}

	return found->map;
}

map_value_t *basic_get_map_value(struct hashmap *map, const char *key)
{
	return hashmap_get(map, &(map_value_t){ .name = key, .name_length = strlen(key) });
}

void basic_free_map_up_value(struct basic_ctx *ctx, up_value *value)
{
	if (value->kind == UP_STR && value->v.s) {
		buddy_free(ctx->allocator, (void *)value->v.s);
	}

	memset(value, 0, sizeof(*value));
}

bool basic_copy_map_up_value(struct basic_ctx *ctx, up_value *dest, const up_value *src)
{
	switch (src->kind) {
		case UP_INT:
			*dest = up_make_int(src->v.i);
			return true;

		case UP_REAL:
			*dest = up_make_real(src->v.r);
			return true;

		case UP_STR:
			*dest = up_make_str(buddy_strdup(ctx->allocator, src->v.s ? src->v.s : ""));
			return dest->v.s != NULL;
	}

	return false;
}

int64_t basic_map(struct basic_ctx *ctx)
{
	struct hashmap *map;
	basic_map_handle_entry entry;

	map = hashmap_new_with_allocator(varmap_malloc, varmap_realloc, varmap_free, sizeof(map_value_t), 0, SEED0, SEED1, map_value_hash, map_value_compare, elfree_map_value, ctx->allocator);

	if (!map) {
		tokenizer_error_print(ctx, "Out of memory creating map");
		return 0;
	}
	memset(&entry, 0, sizeof(entry));
	entry.id = rdtsc();
	entry.map = map;

	hashmap_set(ctx->maps, &entry);
	if (hashmap_oom(ctx->maps)) {
		hashmap_free(map);
		tokenizer_error_print(ctx, "Out of memory setting map entry");
		return 0;
	}

	return entry.id;
}

int64_t basic_maphas(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t handle = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char *key = strval;
	PARAMS_END("MAPHAS", 0);

	struct hashmap *map = basic_get_map_by_handle(ctx, handle);
	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return 0;
	}

	return basic_get_map_value(map, key) ? 1 : 0;
}

int64_t basic_mapget(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t handle = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char *key = strval;
	PARAMS_END("MAPGET", 0);

	struct hashmap *map = basic_get_map_by_handle(ctx, handle);
	map_value_t *entry;

	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return 0;
	}

	entry = basic_get_map_value(map, key);
	if (!entry) {
		tokenizer_error_printf(ctx, "No such MAP key '%s'", key);
		return 0;
	}

	if (entry->value.kind != UP_INT) {
		tokenizer_error_printf(ctx, "MAP key '%s' is not INTEGER", key);
		return 0;
	}

	return entry->value.v.i;
}

void basic_mapgetr(struct basic_ctx *ctx, double* rv)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t handle = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char *key = strval;
	PARAMS_END_VOID("MAPGETR");

	struct hashmap *map = basic_get_map_by_handle(ctx, handle);
	map_value_t *entry;

	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		*rv = 0;
		return;
	}

	entry = basic_get_map_value(map, key);
	if (!entry) {
		tokenizer_error_printf(ctx, "No such MAP key '%s'", key);
		*rv = 0;
		return;
	}

	if (entry->value.kind == UP_INT) {
		*rv = entry->value.v.i;
		return;
	}

	if (entry->value.kind != UP_REAL) {
		tokenizer_error_printf(ctx, "MAP key '%s' is not REAL", key);
		*rv = 0;
		return;
	}

	*rv = entry->value.v.r;
}

char *basic_mapgets(struct basic_ctx *ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t handle = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char *key = strval;
	PARAMS_END("MAPGET$", "");

	struct hashmap *map = basic_get_map_by_handle(ctx, handle);
	map_value_t *entry;

	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return "";
	}

	entry = basic_get_map_value(map, key);
	if (!entry) {
		tokenizer_error_printf(ctx, "No such MAP key '%s'", key);
		return "";
	}

	if (entry->value.kind != UP_STR) {
		tokenizer_error_printf(ctx, "MAP key '%s' is not STRING", key);
		return "";
	}

	return (char *)gc_strdup(ctx, entry->value.v.s ? entry->value.v.s : "");
}

void mapset_statement(struct basic_ctx *ctx)
{
	int64_t handle;
	const char *key;
	struct hashmap *map;
	map_value_t *found;
	map_value_t new_entry;
	up_value value;

	accept_or_return(MAPSET, ctx);
	handle = expr(ctx);
	accept_or_return(COMMA, ctx);
	key = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	up_eval_value(ctx, &value);
	accept_or_return(NEWLINE, ctx);

	map = basic_get_map_by_handle(ctx, handle);
	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return;
	}

	if (value.kind == UP_STR) {
		const char *dup = buddy_strdup(ctx->allocator, value.v.s ? value.v.s : "");
		if (!dup) {
			tokenizer_error_print(ctx, "Out of memory");
			return;
		}
		value.v.s = dup;
	}

	found = basic_get_map_value(map, key);
	if (found) {
		if (found->value.kind != value.kind) {
			if (!(found->value.kind == UP_INT && value.kind == UP_REAL)) {
				if (!(found->value.kind == UP_REAL && value.kind == UP_INT)) {
					basic_free_map_up_value(ctx, &value);
					tokenizer_error_printf(ctx, "Type mismatch for MAP key '%s'", key);
					return;
				}
			}
		}

		basic_free_map_up_value(ctx, &found->value);

		if (!basic_copy_map_up_value(ctx, &found->value, &value)) {
			basic_free_map_up_value(ctx, &value);
			tokenizer_error_print(ctx, "Out of memory");
			return;
		}

		basic_free_map_up_value(ctx, &value);

		hashmap_set(map, found);
		if (hashmap_oom(map)) {
			tokenizer_error_print(ctx, "Out of memory");
		}

		return;
	}

	memset(&new_entry, 0, sizeof(new_entry));
	new_entry.name = buddy_strdup(ctx->allocator, key);
	new_entry.name_length = strlen(key);
	if (!new_entry.name) {
		basic_free_map_up_value(ctx, &value);
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}

	if (!basic_copy_map_up_value(ctx, &new_entry.value, &value)) {
		buddy_free(ctx->allocator, (void *)new_entry.name);
		basic_free_map_up_value(ctx, &value);
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}

	basic_free_map_up_value(ctx, &value);

	hashmap_set(map, &new_entry);
	if (hashmap_oom(map)) {
		basic_free_map_up_value(ctx, &new_entry.value);
		buddy_free(ctx->allocator, (void *)new_entry.name);
		tokenizer_error_print(ctx, "Out of memory");
	}
}

void mapfree_statement(struct basic_ctx *ctx)
{
	basic_map_handle_entry *found;

	accept_or_return(MAPFREE, ctx);
	int64_t handle = expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (handle == 0) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return;
	}

	found = hashmap_delete(ctx->maps, &(basic_map_handle_entry){ .id = handle});
	if (!found) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return;
	}
}

bool mapkeys_iter(const void *item, void *udata)
{
	const map_value_t *entry = item;
	mapkeys_iter_ctx *iter = udata;

	basic_set_string_array_variable(iter->dest, iter->index, entry->name, iter->ctx);
	iter->index++;

	return !iter->ctx->errored;
}

void mapkeys_statement(struct basic_ctx *ctx)
{
	size_t var_length;
	mapkeys_iter_ctx iter;

	accept_or_return(MAPKEYS, ctx);
	int64_t handle = expr(ctx);
	accept_or_return(COMMA, ctx);

	const char* dest = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);

	const char* count_var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(NEWLINE, ctx);

	struct hashmap *map = basic_get_map_by_handle(ctx, handle);
	if (!map) {
		tokenizer_error_print(ctx, "Invalid MAP");
		return;
	}

	size_t count = hashmap_count(map);

	if (!varname_is_string_array_access(ctx, dest)) {
		if (!basic_dim_string_array(dest, count ? (int64_t)count : 1, ctx)) {
			return;
		}
	} else {
		if (!basic_redim_string_array(dest, count ? (int64_t)count : 1, ctx)) {
			return;
		}
	}

	basic_set_int_variable(count_var, (int64_t)count, ctx, false, false);

	if (count == 0) {
		basic_set_string_array_variable(dest, 0, "", ctx);
		return;
	}

	memset(&iter, 0, sizeof(iter));
	iter.ctx = ctx;
	iter.dest = dest;
	iter.index = 0;

	hashmap_scan(map, mapkeys_iter, &iter);
}