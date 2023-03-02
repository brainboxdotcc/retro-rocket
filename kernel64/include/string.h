#ifndef __STRING_H__
#define __STRING_H__

unsigned int strlen(const char* str);

unsigned char tolower(unsigned char input);

int strcmp(const char* s1, const char* s2);

u32 strlcat(char *dst, const char *src, u32 siz);

u32 strlcpy(char *dst, const char *src, u32 siz);

u32 hextoint(const char* n1);

#endif
