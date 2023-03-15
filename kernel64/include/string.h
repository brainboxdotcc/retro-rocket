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

int strncmp(const char* s1, const char* s2, uint32_t n);

char* strchr(const char *s, int c);

uint32_t strlcat(char *dst, const char *src, uint32_t siz);

uint32_t strlcpy(char *dst, const char *src, uint32_t siz);

char* strdup(const char* string);
char* gc_strdup(const char* string);
int gc();

uint64_t hextoint(const char* n1);

unsigned char isdigit(const char x);
unsigned char isxdigit(const char x);

int atoi(const char *s);
int64_t atoll(const char *s, int radix);
uint64_t atoull(const char *s);

int abs(int a);
int labs(int64_t a);

#endif
