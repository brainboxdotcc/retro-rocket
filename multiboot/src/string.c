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

u32int hextoint(const char* n1)
{
	u32int length = strlen(n1);
	u32int result = 0;
	int i = 0, fact = 1;

	if (length)
	{
		if (length>8)
			length = 8;

		for(i = length - 1; i >= 0; i--)
		{
			char digit = tolower(*(n1 + i));
			if ((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f'))
			{
				if (digit >= 97)
					result += (digit - 87) * fact;
				else
					result += (digit - 48) * fact;
				fact = fact << 4;
			}
			else
			{
				return 0;
			}
		}
		return result;
	}

	return 0;
}
