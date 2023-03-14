#include <kernel.h>

void _memset(void *dest, char val, u64 len)
{
	char *temp = (char *)dest;
	for ( ; len != 0; len--) *temp++ = val;
}


void *memcpy(void *dest, const void *src, u64 len)
{
	u64 i;

	/* 64-bit copy whatever we can. This works better on aligned memory areas */
	if ((u64)dest % sizeof(u64) == 0 && (u64)src % sizeof(u64) == 0 && len % sizeof(u64) == 0)
	{

		u64 *d = dest;
		const u64 *s = src;

		for (i=0; i< len / sizeof(u64); i++)
		{
			d[i] = s[i];
		}
	}
	else
	{
		/* Byte-copy the remainder */
		u8 *d = dest;
		const u8 *s = src;

		for (i=0; i < len; i++)
		{
			d[i] = s[i];
		}
	}

	return dest;
}

void *memmove(void *dest, const void *src, u64 n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (u64 i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (u64 i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, u64 n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (u64 i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}