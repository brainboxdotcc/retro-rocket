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

static void *memcpy_reverse(void *dest, const void *src, uint64_t len)
{
	void *ret = dest;
	void *d = (uint8_t *) dest + len - 1;
	const void *s = (const uint8_t *) src + len - 1;

	__asm__ volatile (
		"std\n\t"
		"rep movsb\n\t"
		"cld"
		: "+D"(d), "+S"(s), "+c"(len)
		:
		: "memory"
		);

	return ret;
}

void *memmove(void *dest, const void *src, uint64_t n)
{
	if (src > dest) {
		return memcpy(dest, src, n);
	} else if (src < dest) {
		return memcpy_reverse(dest, src, n);
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

void* memchr(const void *ptr, int c, uint64_t len)
{
	const uint8_t *p = ptr;
	uint8_t val = c;

	while (len--) {
		if (*p == val) {
			return (void*)p;
		}
		p++;
	}

	return NULL;
}