/**
 * @file stdlib.h
 * @author Craig Edwards
 * @brief Standard library stubs
 * @date 2023-05-27
 * @copyright Copyright (c) 2023
 */
#include <stddef.h>
#include <stdint.h>

#pragma once

typedef struct div_t {
	int quot;
	int rem;
} div_t;

typedef struct ldiv_t {
	long int quot;
	long int rem;
} ldiv_t;

typedef char schar_t;

double strtod(const char *str, char **endptr);

long int strtol(const char *str, char **endptr, int base);

unsigned long int strtoul(const char *str, char **endptr, int base);

void abort(void);

int atexit(void (*func)(void));

void exit(int status);

char *getenv(const char *name);

int system(const char *string);

void *bsearch(const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *));

void qsort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*));

div_t div(int numer, int denom);

ldiv_t ldiv(long int numer, long int denom);

int rand(void);

void srand(unsigned int seed);

int mblen(const char *str, size_t n);

size_t mbstowcs(schar_t *pwcs, const char *str, size_t n);

int mbtowc(wchar_t *pwc, const char *str, size_t n);

size_t wcstombs(char *str, const wchar_t *pwcs, size_t n);

int wctomb(char *str, wchar_t wchar);

