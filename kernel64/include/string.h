#ifndef __STRING_H__
#define __STRING_H__

struct gc_str
{
	const char* ptr;
	struct gc_str* next;
};

unsigned int strlen(const char* str);

unsigned char tolower(unsigned char input);

int strcmp(const char* s1, const char* s2);

int strncmp(const char* s1, const char* s2, u32 n);

char* strchr(const char *s, int c);

u32 strlcat(char *dst, const char *src, u32 siz);

u32 strlcpy(char *dst, const char *src, u32 siz);

char* strdup(const char* string);
char* gc_strdup(const char* string);
int gc();

u64 hextoint(const char* n1);

unsigned char isdigit(const char x);
unsigned char isxdigit(const char x);

int atoi(const char *s);
s64 atoll(const char *s, int radix);
u64 atoull(const char *s);

int abs(int a);
int labs(s64 a);

#endif
