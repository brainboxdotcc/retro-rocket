#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Ordered key→value container (red–black tree).
 *
 * Stores scalar keys and values (pointers, integers) as uintptr_t.
 * Keys are kept in strict weak order via a user comparator.
 * Lookups, inserts, and erases are O(log n). In-order iteration is supported.
 */
typedef struct map map;
typedef struct map_iter map_iter;

/**
 * @brief Comparator function for keys.
 *
 * Must return <0 if a < b, 0 if equal, >0 if a > b. The opaque pointer is
 * passed through unchanged to allow custom ordering contexts.
 */
typedef int (*map_keycmp)(uintptr_t a, uintptr_t b, void *opaque);

/**
 * @brief Create a map with the given key comparator.
 * @param cmp   Comparator function (must not be NULL)
 * @param opaque User pointer passed to cmp on every compare (may be NULL)
 * @return Pointer to a new map, or NULL on allocation failure
 */
map *map_create(map_keycmp cmp, void *opaque);

/**
 * @brief Destroy a map and free all nodes.
 * @param m Map to destroy (NULL is ignored)
 */
void map_destroy(map *m);

/**
 * @brief Remove all entries, leaving the map empty.
 * @param m Map to clear
 */
void map_clear(map *m);

/**
 * @brief Number of key→value pairs currently stored.
 * @param m Map to query
 * @return Count of elements
 */
size_t map_size(const map *m);

/**
 * @brief Insert a key→value pair if the key is not present.
 * @param m Map to modify
 * @param key Key to insert
 * @param value Value to store
 * @return 1 if inserted, 0 if key already existed (value unchanged), -1 on OOM
 */
int map_insert(map *m, uintptr_t key, uintptr_t value);

/**
 * @brief Insert or assign: set value whether the key exists or not.
 * @param m Map to modify
 * @param key Key
 * @param value Value to assign
 * @return 1 if inserted, 0 if updated existing, -1 on OOM
 */
int map_insert_or_assign(map *m, uintptr_t key, uintptr_t value);

/**
 * @brief Find a value by key.
 * @param m Map to query
 * @param key Key to look up
 * @param out_value Optional; if non-NULL, receives the found value
 * @return 1 if found (out_value set), 0 if not found
 */
int map_find(const map *m, uintptr_t key, uintptr_t *out_value);

/**
 * @brief Get a pointer to the stored value for an existing key.
 * Useful for in-place updates without a second lookup.
 * @param m Map to modify
 * @param key Key to look up
 * @return Pointer to value if found, NULL otherwise
 */
uintptr_t *map_find_ref(map *m, uintptr_t key);

/**
 * @brief Erase an entry by key.
 * @param m Map to modify
 * @param key Key to erase
 * @return 1 if erased, 0 if key not present
 */
int map_erase(map *m, uintptr_t key);

/**
 * @brief Lower bound: first key >= query.
 * @param m Map to query
 * @param key Query key
 * @param it_out Iterator positioned at lower bound on success
 * @return 1 if a bound exists (iterator valid), 0 if all keys < query (no bound)
 */
int map_lower_bound(const map *m, uintptr_t key, map_iter *it_out);

/**
 * @brief Upper bound: first key > query.
 * @param m Map to query
 * @param key Query key
 * @param it_out Iterator positioned at upper bound on success
 * @return 1 if a bound exists, 0 if all keys <= query
 */
int map_upper_bound(const map *m, uintptr_t key, map_iter *it_out);

/**
 * @brief Begin iterator (smallest key).
 * @param m Map to iterate
 * @param it_out Receives iterator; becomes invalid if map is modified
 * @return 1 if non-empty and iterator valid, 0 if empty
 */
int map_begin(const map *m, map_iter *it_out);

/**
 * @brief End iterator (one past largest). Mostly for symmetry; rarely needed.
 * @param m Map to iterate
 * @param it_out Receives a sentinel end iterator
 */
void map_end(const map *m, map_iter *it_out);

/**
 * @brief Advance iterator to next in-order key.
 * @param it Iterator to advance
 * @return 1 if moved to a valid element, 0 if now at end
 */
int map_next(map_iter *it);

/**
 * @brief Move iterator to previous in-order key.
 * @param it Iterator to retreat
 * @return 1 if moved to a valid element, 0 if now before begin
 */
int map_prev(map_iter *it);

/**
 * @brief Access the key at an iterator.
 * @param it Iterator (must be valid)
 * @return Current key
 */
uintptr_t map_iter_key(const map_iter *it);

/**
 * @brief Access the value at an iterator.
 * @param it Iterator (must be valid)
 * @return Current value
 */
uintptr_t map_iter_value(const map_iter *it);

/**
 * @brief Access a mutable reference to the value at an iterator.
 * @param it Iterator (must be valid)
 * @return Pointer to value
 */
uintptr_t *map_iter_value_ref(map_iter *it);

/**
 * @brief Check whether two iterators are equal (same position).
 * @param a First iterator
 * @param b Second iterator
 * @return 1 if equal, 0 otherwise
 */
int map_iter_equal(const map_iter *a, const map_iter *b);

/**
 * @brief Default numeric comparator: a<b ? -1 : a>b ? +1 : 0.
 * Interprets keys as unsigned 64-bit (via uintptr_t).
 */
int map_cmp_u64(uintptr_t a, uintptr_t b, void *opaque);

/**
 * @brief Default string comparator for const char* keys (strcmp).
 * Keys must be NUL-terminated C strings (stored as pointers in uintptr_t).
 */
int map_cmp_cstr(uintptr_t a, uintptr_t b, void *opaque);

/**
 * @brief Case-insensitive string comparator for const char* (ASCII).
 * Keys must be NUL-terminated C strings (stored as pointers in uintptr_t).
 */
int map_cmp_cstr_nocase(uintptr_t a, uintptr_t b, void *opaque);

/* Convenience wrappers for common pointer / string usage.
 * These just force casts through uintptr_t.
 */

#define map_insert_ptr(m, k, v) \
    map_insert((m), (uintptr_t)(k), (uintptr_t)(v))

#define map_insert_or_assign_ptr(m, k, v) \
    map_insert_or_assign((m), (uintptr_t)(k), (uintptr_t)(v))

#define map_find_ptr(m, k, outp) \
    map_find((m), (uintptr_t)(k), (uintptr_t *)(outp))

#define map_erase_ptr(m, k) \
    map_erase((m), (uintptr_t)(k))

/* For string-keyed maps (const char* keys) */
#define map_insert_str(m, k, v) \
    map_insert((m), (uintptr_t)(const char *)(k), (uintptr_t)(v))

#define map_insert_or_assign_str(m, k, v) \
    map_insert_or_assign((m), (uintptr_t)(const char *)(k), (uintptr_t)(v))

#define map_find_str(m, k, outp) \
    map_find((m), (uintptr_t)(const char *)(k), (uintptr_t *)(outp))

#define map_erase_str(m, k) \
    map_erase((m), (uintptr_t)(const char *)(k))
