#include <kernel.h>

void _memset(void *dest, char val, uint64_t len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}

void memset(void* dest, char val, uint64_t len)
{
	 _memset(dest, val, len);
}


void *memcpy(void *dest, const void *src, uint64_t len)
{
	uint64_t i;

	/* 64-bit copy whatever we can. This works better on aligned memory areas */
	if ((uint64_t)dest % sizeof(uint64_t) == 0 && (uint64_t)src % sizeof(uint64_t) == 0 && len % sizeof(uint64_t) == 0) {
		uint64_t *d = dest;
		const uint64_t *s = src;
		for (i=0; i< len / sizeof(uint64_t); i++) {
			d[i] = s[i];
		}
	} else {
		/* Byte-copy the remainder */
		uint8_t *d = dest;
		const uint8_t *s = src;

		for (i=0; i < len; i++) {
			d[i] = s[i];
		}
	}

	return dest;
}

void *memmove(void *dest, const void *src, uint64_t n) {
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