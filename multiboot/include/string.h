#ifndef __STRING_H__
#define __STRING_H__

#include "kernel.h"

struct gc_str
{
	const char* ptr;
	struct gc_str* next;
};

unsigned int strlen(const char* str);

unsigned char tolower(unsigned char input);

int strcmp(const char* s1, const char* s2);

int strncmp(const char* s1, const char* s2, u32int n);

char* strchr(const char *s, int c);

u32int strlcat(char *dst, const char *src, u32int siz);

u32int strlcpy(char *dst, const char *src, u32int siz);

char* strdup(const char* string);
char* gc_strdup(const char* string);
int gc();

u32int hextoint(const char* n1);

unsigned char isdigit(const char x);

int atoi(const char *s);

int abs(int a);

#endif
