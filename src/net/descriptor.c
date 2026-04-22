#include <kernel.h>
#include <emmintrin.h>

_Static_assert((FD_MAX & (FD_MAX - 1)) == 0, "FD_MAX must be a power of two");
#define FD_WORD_BITS (sizeof(uint64_t) * 8)
#define FD_WORD_COUNT (FD_MAX / FD_WORD_BITS)

static tcp_conn_t* fd[FD_MAX] __attribute__((aligned(16))) = { 0 };
static tcp_error_code_t error_codes[FD_MAX] = { 0 };
static uint64_t fd_used[FD_WORD_COUNT] __attribute__((aligned(16))) = { 0 };

/**
 * @brief Allocate a file descriptor using a bitmap + SSE2 scan.
 *
 * fd_used[] stores allocation state as bits (1 = used, 0 = free).
 * The loop scans two 64-bit words at a time (128 FDs) with SSE2,
 * quickly skipping blocks that are completely full.
 *
 * When a non-full word is found, __builtin_ctzll(~fd_used[word])
 * identifies the first free bit, which maps directly to the FD index.
 *
 * This avoids scanning the fd[] pointer array and keeps the hot path
 * small and cache-friendly.
 */
int tcp_allocate_fd(tcp_conn_t* conn) {
	__m128i all_ones = _mm_set1_epi32(-1);
	for (size_t word = 0; word < FD_WORD_COUNT; word += 2) {
		__m128i v = _mm_load_si128((const __m128i*)&fd_used[word]);
		__m128i eq = _mm_cmpeq_epi32(v, all_ones);
		int mask = _mm_movemask_ps(_mm_castsi128_ps(eq));
		if (mask != 0xf) {
			uint64_t free_bits = ~fd_used[word];
			if (free_bits != 0) {
				unsigned bit = __builtin_ctzll(free_bits);
				size_t index = (word << 6) + bit;
				fd_used[word] |= 1ULL << bit;
				fd[index] = conn;
				return (int)index;
			}
			free_bits = ~fd_used[word + 1];
			if (free_bits != 0) {
				unsigned bit = __builtin_ctzll(free_bits);
				size_t index = ((word + 1) << 6) + bit;
				fd_used[word + 1] |= 1ULL << bit;
				fd[index] = conn;
				return (int)index;
			}
		}
	}
	return -1;
}

void tcp_free_fd(int x) {
	if (x >= 0 && x < FD_MAX) {
		size_t word = (size_t)x >> 6;
		size_t bit = (size_t)x & 63;

		fd[x] = NULL;
		fd_used[word] &= ~(1ULL << bit);
	}
}

tcp_conn_t* tcp_find_by_fd(int x) {
	if (x >= 0 && x < FD_MAX) {
		return fd[x];
	}
	return NULL;
}

void tcp_set_close_code(tcp_conn_t* conn, tcp_error_code_t code) {
	if (!conn) {
		return;
	}
	if (conn->fd < 0 || conn->fd >= FD_MAX) {
		return;
	}

	if (conn->close_code == TCP_ERROR_NONE) {
		conn->close_code = code;
		error_codes[conn->fd] = code;
	}
}

void tcp_set_close_code_by_fd(int this_fd, tcp_error_code_t code) {
	if (this_fd < 0 || this_fd >= FD_MAX) {
		return;
	}

	tcp_conn_t* conn = tcp_find_by_fd(this_fd);
	if (conn && conn->close_code == TCP_ERROR_NONE) {
		conn->close_code = code;
	}
	error_codes[this_fd] = code;
}

tcp_error_code_t tcp_get_close_code(int this_fd) {
	if (this_fd < 0 || this_fd >= FD_MAX) {
		return TCP_ERROR_INVALID_SOCKET;
	}

	return error_codes[this_fd];
}
