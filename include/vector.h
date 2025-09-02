#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Contiguous dynamic array of uintptr_t.
 *
 * Behaves like a simplified C++ std::vector but for scalars
 * (pointers, integers, handles). Elements are stored linearly,
 * cache-friendly. Growth is automatic on push or insert.
 *
 * Order-preserving erase/insert are the defaults. For O(1) removal
 * without preserving order, use vector_erase_swap().
 */
typedef struct vector {
	uintptr_t *data;   /**< Pointer to contiguous storage */
	size_t count;      /**< Number of elements in use */
	size_t capacity;   /**< Allocated capacity in elements */
} vector;

/**
 * @brief Initialise a vector with optional initial capacity.
 * @param v Vector to initialise
 * @param initial_capacity Reserve space for this many elements, 0 = none
 * @return 0 on success, -1 on allocation failure
 */
int vector_init(vector *v, size_t initial_capacity);

/**
 * @brief Free the storage owned by the vector.
 * @param v Vector to free
 */
void vector_free(vector *v);

/**
 * @brief Reset element count to zero, keeping capacity.
 * @param v Vector to clear
 */
void vector_clear(vector *v);

/**
 * @brief Ensure capacity is at least want_cap.
 * @param v Vector to modify
 * @param want_cap Desired minimum capacity
 * @return 0 on success, -1 on allocation failure
 */
int vector_reserve(vector *v, size_t want_cap);

/**
 * @brief Return the current element count.
 * @param v Vector to query
 * @return Number of elements
 */
size_t vector_size(const vector *v);

/**
 * @brief Access the raw storage for iteration or bulk ops.
 * @param v Vector to query
 * @return Pointer to first element
 */
uintptr_t *vector_data(vector *v);

/**
 * @brief Access the raw storage as const.
 * @param v Vector to query
 * @return Const pointer to first element
 */
const uintptr_t *vector_cdata(const vector *v);

/**
 * @brief Get element at index (no bounds check).
 * @param v Vector to query
 * @param i Index
 * @return Element value
 */
uintptr_t vector_get(const vector *v, size_t i);

/**
 * @brief Set element at index (no bounds check).
 * @param v Vector to modify
 * @param i Index
 * @param value Value to assign
 */
void vector_set(vector *v, size_t i, uintptr_t value);

/**
 * @brief Append value at the end, growing if needed.
 * @param v Vector to modify
 * @param value Element to add
 * @return 0 on success, -1 on allocation failure
 */
int vector_push(vector *v, uintptr_t value);

/**
 * @brief Remove and return last element.
 * @param v Vector to modify (must not be empty)
 * @return Last element
 */
uintptr_t vector_pop(vector *v);

/**
 * @brief Insert element at position, shifting tail up.
 * @param v Vector to modify
 * @param i Position (0..count valid)
 * @param value Element to insert
 * @return 0 on success, -1 on allocation failure
 */
int vector_insert_shift(vector *v, size_t i, uintptr_t value);

/**
 * @brief Erase element at position, shifting tail down.
 * @param v Vector to modify
 * @param i Position (0..count-1)
 * @return 0 on success, -1 on invalid index
 */
int vector_erase_shift(vector *v, size_t i);

/**
 * @brief Erase element at position by swapping with last.
 * Order is not preserved, O(1).
 * @param v Vector to modify
 * @param i Position (0..count-1)
 * @return 0 on success, -1 on invalid index
 */
int vector_erase_swap(vector *v, size_t i);

/**
 * @brief Default shorthand for insert (order-preserving).
 * @param v Vector to modify
 * @param i Position (0..count valid)
 * @param value Element to insert
 * @return 0 on success, -1 on allocation failure
 */
int vector_insert(vector *v, size_t i, uintptr_t value);

/**
 * @brief Default shorthand for erase (order-preserving).
 * @param v Vector to modify
 * @param i Position (0..count-1)
 * @return 0 on success, -1 on invalid index
 */
int vector_erase(vector *v, size_t i);

uintptr_t *vector_data(vector *v);

const uintptr_t *vector_cdata(const vector *v);

/* Convenience wrappers for pointer-heavy usage */

#define vector_push_ptr(v, p) \
    vector_push((v), (uintptr_t)(p))

#define vector_get_ptr(v, i, type) \
    ((type)vector_get((v), (i)))

#define vector_set_ptr(v, i, p) \
    vector_set((v), (i), (uintptr_t)(p))
