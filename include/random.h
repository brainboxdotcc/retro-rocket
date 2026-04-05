/**
 * @file random.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 *
 * Random number generation interfaces for Retro Rocket.
 *
 * Provides:
 * - Mersenne Twister (MT19937) pseudo-random generator
 * - Entropy collection helpers
 * - Cryptographically secure random number generation via mbedTLS CTR-DRBG
 */
#pragma once

#include "kernel.h"

#define STATE_VECTOR_LENGTH	624
#define STATE_VECTOR_M		397
#define RAND_MAX_ENTROPY	(size_t)24
#define RAND_MAX		SIZE_MAX

/**
 * Extract a 64-bit big-endian value from an 8-byte buffer.
 *
 * @param p Pointer to at least 8 bytes of data.
 * @return 64-bit value constructed from the buffer.
 */
#define SAMPLE_FROM_BUFFER(p) \
	(((uint64_t)((const uint8_t*)(p))[0] << 56) | \
	((uint64_t)((const uint8_t*)(p))[1] << 48) | \
	((uint64_t)((const uint8_t*)(p))[2] << 40) | \
	((uint64_t)((const uint8_t*)(p))[3] << 32) | \
	((uint64_t)((const uint8_t*)(p))[4] << 24) | \
	((uint64_t)((const uint8_t*)(p))[5] << 16) | \
	((uint64_t)((const uint8_t*)(p))[6] << 8) | \
	((uint64_t)((const uint8_t*)(p))[7]))

/**
 * Mersenne Twister state.
 */
typedef struct mt_rand_t {
	uint32_t mt[STATE_VECTOR_LENGTH];
	int index;
} mt_rand_t;

/**
 * Add entropy to the internal entropy pool.
 *
 * Typically called from interrupt or timing sources. Once sufficient entropy
 * has been collected, the Mersenne Twister is seeded automatically.
 *
 * @param bytes Entropy sample to mix into the pool.
 */
void add_random_entropy(uint64_t bytes);

/**
 * Generate a 32-bit pseudo-random value using MT19937.
 *
 * @param rand Pointer to generator state.
 * @return Pseudo-random 32-bit value.
 */
uint32_t gen_rand_long(mt_rand_t* rand);

/**
 * Generate a pseudo-random double in the range [0, 1].
 *
 * @param rand Pointer to generator state.
 * @return Pseudo-random double.
 */
double gen_rand_double(mt_rand_t* rand);

/**
 * Generate a 64-bit pseudo-random value using MT19937.
 *
 * @param rand Pointer to generator state.
 * @return Pseudo-random 64-bit value.
 */
uint64_t gen_rand_64(mt_rand_t* rand);

/**
 * Generate a 64-bit pseudo-random value using the global MT19937 instance.
 *
 * @return Pseudo-random 64-bit value.
 */
uint64_t mt_rand();

/**
 * Fill a buffer with cryptographically secure random bytes.
 *
 * Uses the global CTR-DRBG instance seeded via the system entropy source.
 *
 * @param buf Destination buffer.
 * @param len Number of bytes to write.
 * @return true on success, false on failure.
 */
bool csprng_fill(void *buf, size_t len);

/**
 * Generate a cryptographically secure random integer within a range.
 *
 * Produces a uniformly distributed value in the inclusive range [x, y].
 * Rejection sampling is used to eliminate modulo bias.
 *
 * @param x Lower bound (inclusive).
 * @param y Upper bound (inclusive).
 * @param out Output pointer for result.
 * @return true on success, false on failure.
 */
bool csprng_range(int64_t x, int64_t y, int64_t *out);

/**
 * Fill a buffer with a cryptographically secure random string using a custom alphabet.
 *
 * Each character is selected independently from the supplied alphabet using a
 * uniform distribution. Rejection sampling is used internally (via
 * csprng_range) to eliminate modulo bias, ensuring each character in the
 * alphabet is equally likely.
 *
 * The output is always null-terminated on success.
 *
 * The caller must provide:
 * - a buffer large enough to hold len characters plus the terminating null byte
 * - a valid alphabet with at least one character
 *
 * @param out Destination buffer.
 * @param len Number of characters to generate, excluding the terminating null byte.
 * @param alphabet Pointer to the character set to sample from.
 * @param alphabet_len Number of characters in the alphabet.
 * @return true on success, false on failure.
 */
bool csprng_string_from_alphabet(char *out, size_t len, const char *alphabet, size_t alphabet_len);

/**
 * Generate a pseudo-random integer within a range using MT19937.
 *
 * Produces a uniformly distributed value in the inclusive range [x, y].
 * Rejection sampling is used to eliminate modulo bias, ensuring each value
 * in the range is equally likely.
 *
 * This function is not cryptographically secure and must not be used for
 * security-sensitive purposes. It is suitable for gameplay, simulation, and
 * general-purpose pseudo-random use.
 *
 * @param x Lower bound (inclusive).
 * @param y Upper bound (inclusive).
 * @param out Output pointer for result.
 * @return true on success, false on failure.
 */
bool mt_rand_range(int64_t x, int64_t y, int64_t *out);
