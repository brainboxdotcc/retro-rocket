#include <kernel.h>

void* memset(void* dest, char val, uint64_t len)
{
	void *ret = dest;
	__asm__ volatile (
		"rep stosb"
		: "+D"(dest), "+c"(len)
		: "a"(val)
		: "memory"
		);
	return ret;
}

void* memcpy(void *dest, const void *src, uint64_t len)
{
	void *d = dest;
	__asm__ volatile (
		"rep movsb"
		: "+D"(d), "+S"(src), "+c"(len)
		:
		: "memory"
		);
	return dest;
}

void* memmove(void *dest, const void *src, uint64_t n) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	if (src > dest) {
		for (uint64_t i = 0; i < n; i++) {
			pdest[i] = psrc[i];
		}
	} else if (src < dest) {
		for (uint64_t i = n; i > 0; i--) {
			pdest[i-1] = psrc[i-1];
		}
	}

	return dest;
}

int memcmp(const void *s1, const void *s2, uint64_t n) {
	const uint8_t *p1 = (const uint8_t *)s1;
	const uint8_t *p2 = (const uint8_t *)s2;

	for (size_t i = 0; i < n; i++) {
		int d = (int)p1[i] - (int)p2[i];
		if (d) {
			return d;
		}
	}

	return 0;
}

size_t memrev(char* buf, size_t n)
{
	size_t i;
	for (i = 0; i < n/2; ++i) {
		const char x = buf[i];
		buf[i] = buf[n - i - 1];
		buf[n - i - 1] = x;
	}
	return i;
}
