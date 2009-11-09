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
