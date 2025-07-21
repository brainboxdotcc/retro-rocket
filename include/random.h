/**
 * @file rtl8139.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

#define STATE_VECTOR_LENGTH	624
#define STATE_VECTOR_M		397
#define RAND_MAX_ENTROPY	(size_t)24
#define RAND_MAX		SIZE_MAX

typedef struct mt_rand_t {
	uint32_t mt[STATE_VECTOR_LENGTH];
	int index;
} mt_rand_t;


void add_random_entropy(uint64_t bytes);
uint32_t gen_rand_long(mt_rand_t* rand);
double gen_rand_double(mt_rand_t* rand);
uint64_t gen_rand_64(mt_rand_t* rand);
uint64_t mt_rand();
