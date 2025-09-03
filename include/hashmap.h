/**
 * @file hashmap.h
 * @brief Open-addressed hashmap (Robin Hood), with allocator udata support.
 *
 * Updated by Craig Edwards, September 2025, to allow `udata` to be passed to
 * the allocator functions. This enables contextual/arena allocators (e.g., per
 * BASIC interpreter context) without altering external call sites.
 *
 * Based on tidwall hashmap, copyright 2020 Joshua J Baker. All rights
 * reserved. Use of this source code is governed by an MIT-style licence that
 * can be found in the LICENSE file. https://github.com/tidwall/hashmap.c
 */
#pragma once

#include "kernel.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct hashmap;

/* --------------------------------------------------------------------------
 * Callback typedefs
 * -------------------------------------------------------------------------- */

/**
 * @brief Allocate a new block.
 * @param size  Bytes requested.
 * @param udata User data pointer supplied at map construction.
 * @return Pointer to allocated block, or NULL on failure.
 */
typedef void *(*hashmap_allocator)(size_t size, void *udata);

/**
 * @brief Resize an existing block.
 * @param ptr   Existing allocation (or NULL to behave like malloc).
 * @param size  New size in bytes.
 * @param udata User data pointer supplied at map construction.
 * @return Pointer to (possibly moved) block, or NULL on failure.
 */
typedef void *(*hashmap_reallocator)(void *ptr, size_t size, void *udata);

/**
 * @brief Release an allocated block.
 * @param ptr   Block to free (may be NULL).
 * @param udata User data pointer supplied at map construction.
 */
typedef void  (*hashmap_releaser)(const void *ptr, void *udata);

/**
 * @brief Hash an item.
 * @param item  Pointer to key/item.
 * @param seed0 First 64-bit seed.
 * @param seed1 Second 64-bit seed.
 * @return 64-bit hash value.
 */
typedef uint64_t (*hashmap_hash_fn)(const void *item, uint64_t seed0, uint64_t seed1);

/**
 * @brief Compare two items.
 * @param a     First item.
 * @param b     Second item.
 * @param udata User data pointer supplied at map construction.
 * @return <0 if a<b, 0 if equal, >0 if a>b.
 */
typedef int (*hashmap_compare_fn)(const void *a, const void *b, void *udata);

/**
 * @brief Optional element destructor for items stored by value.
 * @param item  Element to dispose.
 *
 * Note: This is only for element-internal resources. The map storage itself is
 * freed by the map’s allocator callbacks.
 */
typedef void (*hashmap_elfree_fn)(const void *item, void *udata);

/**
 * @brief Iterator callback for scanning all items.
 * @param item  Current element.
 * @param udata User data pointer supplied at call site.
 * @return true to continue, false to stop.
 */
typedef bool (*hashmap_iter_fn)(const void *item, void *udata);


/* --------------------------------------------------------------------------
 * Creation / destruction
 * -------------------------------------------------------------------------- */

/**
 * @brief Create a new hashmap with standard kernel allocators.
 *
 * @param elsize Size in bytes of each element stored by the map.
 * @param cap    Initial capacity hint (0 defaults to 16).
 * @param seed0  First 64-bit hash seed.
 * @param seed1  Second 64-bit hash seed.
 * @param hash   Hash function for items.
 * @param compare Comparison function for items.
 * @param elfree Optional element destructor (may be NULL).
 * @param udata  User data passed to `compare` (and may be used internally).
 * @return Handle to a new hashmap, or NULL on OOM.
 */
struct hashmap *hashmap_new(size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1, hashmap_hash_fn hash, hashmap_compare_fn compare, hashmap_elfree_fn elfree, void *udata);

/**
 * @brief Create a new hashmap with custom allocators.
 *
 * @param _malloc  Allocator callback (required).
 * @param _realloc Reallocator callback (required).
 * @param _free    Releaser callback (required).
 * @param elsize   Size in bytes of each element stored by the map.
 * @param cap      Initial capacity hint (0 defaults to 16).
 * @param seed0    First 64-bit hash seed.
 * @param seed1    Second 64-bit hash seed.
 * @param hash     Hash function for items.
 * @param compare  Comparison function for items.
 * @param elfree   Optional element destructor (may be NULL).
 * @param udata    User data passed to allocator callbacks and `compare`.
 * @return Handle to a new hashmap, or NULL on OOM.
 */
struct hashmap *hashmap_new_with_allocator(hashmap_allocator _malloc, hashmap_reallocator _realloc, hashmap_releaser _free, size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1, hashmap_hash_fn hash, hashmap_compare_fn compare, hashmap_elfree_fn elfree, void *udata);

/**
 * @brief Free a hashmap and its storage.
 *
 * Calls `elfree` for each element if provided, then releases internal buffers
 * via the map’s allocator callbacks.
 *
 * @param map Hashmap handle (may be NULL).
 */
void hashmap_free(struct hashmap *map);

/**
 * @brief Clear all items from the map.
 *
 * Each element is passed to `elfree` (if provided). If `update_cap` is true,
 * internal capacity is trimmed to the current number of buckets to avoid new
 * allocations on the clear.
 *
 * @param map        Hashmap handle.
 * @param update_cap When true, shrink capacity to current bucket count.
 */
void hashmap_clear(struct hashmap *map, bool update_cap);

/**
 * @brief Get the number of stored items.
 * @param map Hashmap handle.
 * @return Item count.
 */
size_t hashmap_count(struct hashmap *map);

/**
 * @brief Test whether the previous `hashmap_set` failed due to OOM.
 * @param map Hashmap handle.
 * @return true if the last set operation ran out of memory.
 */
bool hashmap_oom(struct hashmap *map);

/**
 * @brief Look up an item by key.
 * @param map  Hashmap handle.
 * @param item Key to search for.
 * @return Pointer to stored item, or NULL if not found.
 */
void *hashmap_get(struct hashmap *map, const void *item);

/**
 * @brief Insert or replace an item.
 * @param map  Hashmap handle.
 * @param item Item to insert (by value).
 * @return Pointer to the replaced item if it existed, otherwise NULL.
 *
 * On allocation failure returns NULL and sets the sticky OOM flag
 * (see `hashmap_oom`).
 */
void *hashmap_set(struct hashmap *map, const void *item);

/**
 * @brief Remove an item by key.
 * @param map  Hashmap handle.
 * @param item Key to delete.
 * @return Pointer to the removed item, or NULL if not found.
 */
void *hashmap_delete(struct hashmap *map, void *item);

/**
 * @brief Probe a raw bucket position.
 * @param map      Hashmap handle.
 * @param position Bucket index (will be reduced modulo bucket count).
 * @return Pointer to stored item at that bucket, or NULL.
 */
void *hashmap_probe(struct hashmap *map, uint64_t position);

/**
 * @brief Iterate over every item in the map.
 * @param map   Hashmap handle.
 * @param iter  Callback invoked for each item; return false to stop early.
 * @param udata User data passed to `iter`.
 * @return false if iteration was stopped early, true otherwise.
 */
bool hashmap_scan(struct hashmap *map, hashmap_iter_fn iter, void *udata);

/**
 * @brief Cursor-based iterator over items.
 * @param map  Hashmap handle.
 * @param i    Cursor (initialise to 0; updated on each call).
 * @param item Out: pointer to the current stored item.
 * @return true if an item was produced; false if iteration is complete.
 *
 * Note: If `hashmap_delete` is called during iteration, the bucket layout may
 * change. Reset the cursor to 0 to restart iteration safely.
 */
bool hashmap_iter(struct hashmap *map, size_t *i, void **item);

/**
 * @brief SipHash-2-4.
 * @param data  Input buffer.
 * @param len   Length in bytes.
 * @param seed0 First 64-bit seed.
 * @param seed1 Second 64-bit seed.
 * @return 64-bit hash value.
 */
uint64_t hashmap_sip(const void *data, size_t len, uint64_t seed0, uint64_t seed1);

/**
 * @brief Murmur3_86_128 (folded to 64 bits).
 * @param data  Input buffer.
 * @param len   Length in bytes.
 * @param seed0 First 64-bit seed.
 * @param seed1 Second 64-bit seed.
 * @return 64-bit hash value.
 */
uint64_t hashmap_murmur(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
