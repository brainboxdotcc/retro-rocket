#include <kernel.h>
/*
 * @brief strtoul and strtod are taken from OpenBSD. Licensed under the BSD license.
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

double strtod(const char *str, char **endptr)
{
	double out;
	if (str && atof(str, &out)) {
		if (endptr) {
			*endptr = (char*)str + strlen(str);
		}
		return out;
	}
	return 0;
}

long int strtol(const char *nptr, char **endptr, int base)
{
	const char *s;
	long acc, cutoff;
	int c;
	int neg, any, cutlim;

	if (base < 0 || base == 1 || base > 36) {
		if (endptr != 0)
			*endptr = (char *)nptr;
		return 0;
	}

	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) && c == '0' &&
	    (*s == 'x' || *s == 'X') && isxdigit((unsigned char)s[1])) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	cutoff = neg ? LONG_MIN : LONG_MAX;
	cutlim = cutoff % base;
	cutoff /= base;
	if (neg) {
		if (cutlim > 0) {
			cutlim -= base;
			cutoff += 1;
		}
		cutlim = -cutlim;
	}
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (neg) {
			if (acc < cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = LONG_MIN;
			} else {
				any = 1;
				acc *= base;
				acc -= c;
			}
		} else {
			if (acc > cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = LONG_MAX;
			} else {
				any = 1;
				acc *= base;
				acc += c;
			}
		}
	}
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
	const char *s;
	unsigned long acc, cutoff;
	int c;
	int neg, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	if (base < 0 || base == 1 || base > 36) {
		if (endptr != 0)
			*endptr = (char *)nptr;
		return 0;
	}

	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) && c == '0' &&
	    (*s == 'x' || *s == 'X') && isxdigit((unsigned char)s[1])) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = ULONG_MAX;
		} else {
			any = 1;
			acc *= (unsigned long)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

void abort(void)
{
	assert(1, "abort() called");
}

int atexit(void (*func)(void))
{
	kprintf("Invalid stdlib stub: atexit()");
	return -1;
}

void exit(int status)
{
	kprintf("Invalid stdlib stub: exit()");
}

char *getenv(const char *name)
{
	kprintf("Invalid stdlib stub: getenv()");
	return NULL;
}

int system(const char *string)
{
	kprintf("Invalid stdlib stub: system()");
	return -1;
}

void *bsearch(const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *))
{
	kprintf("Invalid stdlib stub: bsearch()");
	return NULL;
}

void qsort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*))
{
	kprintf("Invalid stdlib stub: qsort()");
}

div_t div(int numer, int denom)
{
	return (div_t){
		.quot = numer / denom,
		.rem = numer % denom
	};
}

ldiv_t ldiv(long int numer, long int denom)
{
	return (ldiv_t){
		.quot = numer / denom,
		.rem = numer % denom
	};
}

int rand(void)
{
	kprintf("Invalid stdlib stub: rand()");
	return 0;
}

void srand(unsigned int seed)
{
		kprintf("Invalid stdlib stub: srand()");
}

int mblen(const char *str, size_t n)
{
	return 0;
}

size_t mbstowcs(schar_t *pwcs, const char *str, size_t n)
{
	kprintf("Invalid stdlib stub: mbstowcs()");
	return 0;
}

int mbtowc(wchar_t *pwc, const char *str, size_t n)
{
	kprintf("Invalid stdlib stub: mbtowc()");
	return 0;
}

size_t wcstombs(char *str, const wchar_t *pwcs, size_t n)
{
	kprintf("Invalid stdlib stub: wcstombs()");
	return 0;
}

int wctomb(char *str, wchar_t wchar)
{
	kprintf("Invalid stdlib stub: wctomb()");
	return 0;
}

