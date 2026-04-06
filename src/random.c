#include <kernel.h>
#include "mbedtls/ctr_drbg.h"

/* An implementation of the MT19937 Algorithm for the Mersenne Twister
 * by Evan Sultanik.  Based upon the pseudocode in: M. Matsumoto and
 * T. Nishimura, "Mersenne Twister: A 623-dimensionally
 * equidistributed uniform pseudorandom number generator," ACM
 * Transactions on Modeling and Computer Simulation Vol. 8, No. 1,
 * January pp.3-30 1998.
 *
 * http://www.sultanik.com/Mersenne_twister
 */

uint64_t entropy[RAND_MAX_ENTROPY] = { 0 };
size_t entropy_offset = 0;
bool random_ready = false;
mt_rand_t random_device;

#define UPPER_MASK		0x80000000
#define LOWER_MASK		0x7fffffff
#define TEMPERING_MASK_B	0x9d2c5680
#define TEMPERING_MASK_C	0xefc60000

inline static void mt_seed_rand(mt_rand_t* rand, uint32_t seed) {
	/* set initial seeds to mt[STATE_VECTOR_LENGTH] using the generator
	* from Line 25 of Table 1 in: Donald Knuth, "The Art of Computer
	* Programming," Vol. 2 (2nd Ed.) pp.102.
	*/
	rand->mt[0] = seed & 0xffffffff;
	for(rand->index=1; rand->index<STATE_VECTOR_LENGTH; rand->index++) {
		rand->mt[rand->index] = (6069 * rand->mt[rand->index-1]) & 0xffffffff;
	}
}

/**
* Creates a new random number generator from a given seed.
*/
mt_rand_t seed_rand(uint32_t seed) {
	mt_rand_t rand;
	mt_seed_rand(&rand, seed);
	return rand;
}

/**
 * Generates a pseudo-randomly generated long.
 */
uint32_t gen_rand_long(mt_rand_t* rand)
{
	uint32_t y;
	static uint32_t mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
	if (rand->index >= STATE_VECTOR_LENGTH || rand->index < 0) {
		/* generate STATE_VECTOR_LENGTH words at a time */
		int kk;
		if (rand->index >= STATE_VECTOR_LENGTH+1 || rand->index < 0) {
			mt_seed_rand(rand, 4357);
		}
		for(kk=0; kk<STATE_VECTOR_LENGTH-STATE_VECTOR_M; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
		}
		for(; kk<STATE_VECTOR_LENGTH-1; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+(STATE_VECTOR_M-STATE_VECTOR_LENGTH)] ^ (y >> 1) ^ mag[y & 0x1];
		}
		y = (rand->mt[STATE_VECTOR_LENGTH-1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
		rand->mt[STATE_VECTOR_LENGTH-1] = rand->mt[STATE_VECTOR_M-1] ^ (y >> 1) ^ mag[y & 0x1];
		rand->index = 0;
	}
	y = rand->mt[rand->index++];
	y ^= (y >> 11);
	y ^= (y << 7) & TEMPERING_MASK_B;
	y ^= (y << 15) & TEMPERING_MASK_C;
	y ^= (y >> 18);
	return y;
}

uint64_t gen_rand_64(mt_rand_t* rand)
{
	uint64_t lower = gen_rand_long(rand), upper = gen_rand_long(rand);
	return upper << 32 | lower;
}

uint64_t mt_rand()
{
	return gen_rand_64(&random_device);
}

bool mt_rand_range(int64_t x, int64_t y, int64_t *out) {
	uint64_t r;
	uint64_t ux;
	uint64_t uy;
	uint64_t lo;
	uint64_t hi;
	uint64_t span;
	uint64_t count;
	uint64_t threshold;
	const uint64_t sign_bit = (uint64_t) 1 << 63;

	if (out == NULL) {
		return false;
	}

	ux = ((uint64_t) x) ^ sign_bit;
	uy = ((uint64_t) y) ^ sign_bit;

	if (ux <= uy) {
		lo = ux;
		hi = uy;
	} else {
		lo = uy;
		hi = ux;
	}

	span = hi - lo;
	count = span + 1;

	if (count == 0) {
		r = mt_rand();
		*out = (int64_t) (r ^ sign_bit);
		return true;
	}

	threshold = ((uint64_t) 0 - count) % count;

	for (;;) {
		r = mt_rand();

		if (r >= threshold) {
			uint64_t mapped = lo + (r % count);
			*out = (int64_t) (mapped ^ sign_bit);
			return true;
		}
	}
}

/**
 * Generates a pseudo-randomly generated double in the range [0..1].
 */
double gen_rand_double(mt_rand_t* rand) {
	return ((double)gen_rand_long(rand) / (uint32_t)0xffffffff);
}

void add_random_entropy(uint64_t bytes)
{
	if (bytes == 0) {
		return;
	}
	entropy[entropy_offset] = (bytes ^ rdtsc()) | ((rdtsc() << 32) * entropy_offset);
	if (++entropy_offset >= RAND_MAX_ENTROPY && !random_ready) {
		random_ready = true;
		dprintf("RANDOM: Entropy filled\n");
		random_device = seed_rand(entropy[entropy[0] % RAND_MAX_ENTROPY]);
	}
	entropy_offset %= RAND_MAX_ENTROPY;
}

bool csprng_fill(void *buf, size_t len) {
	if (buf == NULL) {
		return false;
	}
	void* rng = get_random_context();
	if (rng == NULL) {
		return false;
	}
	return mbedtls_ctr_drbg_random(rng, buf, len) == 0;
}

bool csprng_range(int64_t x, int64_t y, int64_t *out) {
	uint64_t r;
	uint64_t ux = ((uint64_t) x) ^ (1ULL << 63);
	uint64_t uy = ((uint64_t) y) ^ (1ULL << 63);
	uint64_t lo = (ux < uy) ? ux : uy;
	uint64_t hi = (ux < uy) ? uy : ux;
	uint64_t span = hi - lo;
	uint64_t count = span + 1;

	if (out == NULL) {
		return false;
	}
	void* rng = get_random_context();
	if (rng == NULL) {
		return false;
	}

	if (count == 0) {
		if (mbedtls_ctr_drbg_random(rng, (unsigned char *) &r, sizeof(r)) != 0) {
			return false;
		}
		*out = (int64_t) (r ^ (1ULL << 63));
		return true;
	}

	uint64_t threshold = ((uint64_t) 0 - count) % count;

	for (;;) {
		if (mbedtls_ctr_drbg_random(rng, (unsigned char *) &r, sizeof(r)) != 0) {
			return false;
		}
		if (r >= threshold) {
			uint64_t mapped = lo + (r % count);
			*out = (int64_t) (mapped ^ (1ULL << 63));
			return true;
		}
	}
}

bool csprng_string_from_alphabet(char *out, size_t len, const char *alphabet, size_t alphabet_len) {
	size_t i;
	int64_t idx;

	if (out == NULL || alphabet == NULL || alphabet_len == 0) {
		return false;
	}

	for (i = 0; i < len; i++) {
		if (!csprng_range(0, (int64_t) alphabet_len - 1, &idx)) {
			out[i] = '\0';
			return false;
		}

		out[i] = alphabet[idx];
	}

	out[len] = '\0';
	return true;
}
