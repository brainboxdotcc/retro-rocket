/**
 * @file data.h
 * @brief DATA/DATASET support for BASIC runtime.
 * @copyright (c) 2009-2026 Craig Edwards
 *
 * Provides storage and access for DATA values and DATASET offsets.
 * DATA values are parsed at load time into a contiguous array.
 * DATASET records positions within that array.
 */
#pragma once
#include <stdint.h>
#include "basic/unified_expression.h"

struct basic_ctx;

/**
 * @brief Contiguous store of DATA values.
 *
 * Holds all values parsed from DATA statements in order.
 * Values are accessed sequentially via data_offset.
 */
typedef struct data_store {
	up_value* values;   /**< Array of DATA values */
	size_t length;      /**< Number of stored values */
} data_store;

/**
 * @brief Collection of DATASET offsets.
 *
 * Stores zero-based positions into the DATA stream.
 */
typedef struct data_sets {
	int64_t* offsets;   /**< Array of offsets into data_store */
	size_t length;      /**< Number of dataset entries */
} data_sets;

/**
 * @brief Populate DATA and DATASET structures from program text.
 *
 * Scans the program, extracts DATA values, and records DATASET offsets.
 */
void fill_datastores(struct basic_ctx* ctx);

/**
 * @brief Free all allocated DATA and DATASET storage.
 */
void free_datastores(struct basic_ctx* ctx);

/**
 * @brief Handle DATASET statement.
 *
 * Records the current DATA offset and assigns it to a variable.
 */
void dataset_statement(struct basic_ctx* ctx);

/**
 * @brief Handle DATA statement.
 *
 * Parses constant values and appends them to the DATA store.
 */
void data_statement(struct basic_ctx* ctx);

/**
 * @brief Handle RESTORE statement.
 *
 * Resets or sets the DATA read offset.
 */
void restore_statement(struct basic_ctx* ctx);

/**
 * @brief Read next DATA value as integer.
 *
 * @return Next integer value.
 */
int64_t basic_dataread(struct basic_ctx* ctx);

/**
 * @brief Read next DATA value as real.
 *
 * Accepts integer or real values. Integers are promoted to real.
 *
 * @param[out] rv Destination for the result.
 */
void basic_dataread_real(struct basic_ctx* ctx, double* rv);

/**
 * @brief Read next DATA value as string.
 *
 * @return Newly allocated string (GC-managed).
 */
char* basic_dataread_string(struct basic_ctx* ctx);
