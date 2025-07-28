#include <kernel.h>

void* memset(void* dest, char val, uint64_t len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
	return dest;
}

static inline void* movsb(void* dst, const void* src, size_t size)
{
	void *copy = dst;
	__asm__ volatile("rep movsb" : "+D"(copy), "+S"(src), "+c"(size) : : "memory");
	return dst;
}

static inline void* movsl(void* dst, const void* src, size_t size)
{
	void *copy = dst;
	__asm__ volatile("rep movsl" : "+D"(copy), "+S"(src), "+c"(size) : : "memory");
	return dst;
}

static inline void* movsw(void* dst, const void* src, size_t size)
{
	void *copy = dst;
	__asm__ volatile("rep movsw" : "+D"(copy), "+S"(src), "+c"(size) : : "memory");
	return dst;
}

static inline void* movsq(void* dst, const void* src, size_t size)
{
	void *copy = dst;
	__asm__ volatile("rep movsq" : "+D"(copy), "+S"(src), "+c"(size) : : "memory");
	return dst;
}

void* memcpy(void *dest, const void *src, uint64_t len)
{
	if (len == 0) {
		return dest;
	} else if ((((size_t)dest & 7) == 0) && (((size_t)src & 7) == 0) && ((len & 7) == 0)) {
		return movsq(dest, src, len >> 3);
	} else if ((((size_t)dest & 3) == 0) && (((size_t)src & 3) == 0) && ((len & 3) == 0)) {
		return movsl(dest, src, len >> 2);
	} else if ((((size_t)dest & 1) == 0) && (((size_t)src & 1) == 0) && ((len & 1) == 0)) {
		return movsw(dest, src, len >> 1);
	} else {
		return movsb(dest, src, len);
	}
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

	for (uint64_t i = 0; i < n; i++) {
		if (p1[i] != p2[i]) {
			return p1[i] < p2[i] ? -1 : 1;
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
