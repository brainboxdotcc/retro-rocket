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
