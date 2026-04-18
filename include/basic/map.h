#pragma once
#include <stdint.h>
#include <basic/unified_expression.h>

/**
 * @brief Single key/value entry stored in a BASIC MAP
 *
 * name is heap-allocated and owned by the map
 * value follows up_value semantics (deep-copied on insert)
 */
typedef struct map_value {
	const char* name;
	size_t name_length;
	up_value value;
} map_value_t;

/**
 * @brief Entry in the global MAP handle table
 *
 * Maps an integer handle to a hashmap instance
 */
typedef struct basic_map_handle_entry {
	int64_t id;
	struct hashmap *map;
} basic_map_handle_entry;

/**
 * @brief Iteration context for MAPKEYS
 *
 * Used when scanning a map to populate a BASIC string array
 */
typedef struct mapkeys_iter_ctx {
	struct basic_ctx *ctx;
	const char *dest;
	int64_t index;
} mapkeys_iter_ctx;

/**
 * @brief Hash function for map handles
 */
uint64_t int64_hash(const void *item, uint64_t seed0, uint64_t seed1);

/**
 * @brief Comparison function for map handles
 */
int int64_compare(const void *a, const void *b, void *udata);

/**
 * @brief Destructor for map handle entries
 *
 * Frees the underlying hashmap
 */
void elfree_map_handle_entry(const void *item, void *udata);

/**
 * @brief Create a new MAP and return its handle
 *
 * Returns 0 on failure
 */
int64_t basic_map(struct basic_ctx *ctx);

/**
 * @brief MAPSET statement handler
 *
 * Sets or replaces a key/value pair in a map
 */
void mapset_statement(struct basic_ctx *ctx);

/**
 * @brief MAPFREE statement handler
 *
 * Deletes a map by handle
 */
void mapfree_statement(struct basic_ctx *ctx);

/**
 * @brief MAPKEYS statement handler
 *
 * Writes all keys into a string array and returns count
 */
void mapkeys_statement(struct basic_ctx *ctx);

/**
 * @brief MAPGET$ function
 *
 * Returns a string value for a key
 */
char *basic_mapgets(struct basic_ctx *ctx);

/**
 * @brief MAPGETR function
 *
 * Returns a real value for a key
 */
void basic_mapgetr(struct basic_ctx *ctx, double* rv);

/**
 * @brief MAPGET function
 *
 * Returns an integer value for a key
 */
int64_t basic_mapget(struct basic_ctx *ctx);

/**
 * @brief MAPHAS function
 *
 * Returns non-zero if key exists in map
 */
int64_t basic_maphas(struct basic_ctx *ctx);
