/**
 * @file string.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct gc_str
{
	const char* ptr;
	struct gc_str* next;
};

unsigned int strlen(const char* str);

int strcmp(const char* s1, const char* s2);

int stricmp(const char* s1, const char* s2);

int strncmp(const char* s1, const char* s2, uint32_t n);

int strnicmp(const char* s1, const char* s2, uint32_t n);

char toupper(char low);

char tolower(char low);

int isalnum(const char x);

bool isspace(const char x);

bool isalpha(const char x);

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
bool atof(const char* s, double* a);

int abs(int a);
int64_t labs(int64_t a);

size_t strrev(char* s);

int do_atoi(int64_t* dst, char* target, unsigned radix);
int do_itoa(int64_t target, char* buf, unsigned radix);
