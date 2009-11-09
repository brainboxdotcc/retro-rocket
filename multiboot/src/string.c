#include "../include/string.h"

unsigned int strlen(const char* str)
{
	unsigned int len = 0;
	for(; *str; ++str)
		len++;
	return len;
}

unsigned char tolower(unsigned char input)
{
	return (input | 0x20);
}

int strcmp(const char* s1, const char* s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}
