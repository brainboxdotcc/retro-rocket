#include <kernel.h>

gc_str_t* gc_list = NULL;

unsigned int strlen(const char* str) {
	size_t rcx;
	/* Search for AL=0 from RDI, RCX = ~0; after SCASB, bytes scanned = (~rcx) - 1. */
	__asm__ volatile (
		"xor %%eax, %%eax\n\t"
		"mov $-1, %%rcx\n\t"
		"repne scasb"
		: "=c"(rcx), "+D"(str)
		: /* AL already zeroed */
		: "rax", "memory"
	);
	return (unsigned int)((~rcx) - 1);
}

void strtolower(char *s) {
	if (!s) {
		return;
	}
	for (; *s; ++s) {
		*s = tolower(*s);
	}
}

int strcmp(const char* s1, const char* s2)
{
	if (!s1 || !s2) {
		return (int)s1 - (int)s2;
	}
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return 0;
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

int stricmp(const char* s1, const char* s2)
{
	if (!s1 || !s2) {
		return (int)s1 - (int)s2;
	}
	while (toupper(*s1) == toupper(*s2++))
		if (*s1++ == 0)
			return 0;
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

char* strchr(const char *s, int c)
{
	if (!s) {
		return NULL;
	}
	for (; *s; ++s) {
		if (*s == c) {
			return (char*) s;
		}
	}
	return NULL;
}

int abs(int a) {
	uint32_t mask = ~((a >> 31) & 1) + 1;
	return (a ^ mask) - mask;
}

int64_t labs(int64_t a) {
	uint64_t mask = ~((a >> 63) & 1) + 1;
	return (a ^ mask) - mask;
}

int strnicmp(const char* s1, const char* s2, uint32_t n)
{
	if (!s1 || !s2) {
		return (int)s1 - (int)s2;
	}
	if (n == 0)
		return 0;
	do {
		if (toupper(*s1) != toupper(*s2++))
			return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}

#include <stdint.h>

__attribute__((hot)) int strncmp(const char *s1, const char *s2, uint32_t n)
{
	if (!s1 || !s2) {
		return (int)s1 - (int)s2; /* preserve your NULL behaviour */
	}

	if (s1 == s2 || n == 0) {
		return 0;
	}

	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (n--) {
		unsigned char a = *p1++;
		unsigned char b = *p2++;

		/* Common case: bytes equal and not NUL → continue. */
		if (__builtin_expect(a == b, 1)) {
			if (__builtin_expect(a != 0u, 1)) {
				continue;
			}
			return 0; /* both NUL at same position */
		}

		/* First difference decides the result. */
		return (int)a - (int)b;
	}

	return 0;
}


uint64_t hextoint(const char* n1)
{
	if (!n1) {
		return 0;
	}

	uint32_t length = strlen(n1);
	uint64_t result = 0;
	int i = 0, fact = 1;

	if (length) {
		if (length > 16) {
			length = 16;
		}

		for(i = length - 1; i >= 0; i--) {
			char digit = tolower(*(n1 + i));
			if ((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f')) {
				if (digit >= 97) {
					result += (digit - 87) * fact;
				} else {
					result += (digit - 48) * fact;
				}
				fact <<= 4;
			} else {
				return 0;
			}
		}
		return result;
	}

	return 0;
}

uint32_t strlcat(char *dst, const char *src, uint32_t siz)
{
	if (!src || !dst) {
		return 0;
	}

	char *d = dst;
	const char *s = src;
	uint32_t n = siz, dlen;

	while (n-- != 0 && *d != '\0') {
		d++;
	}

	dlen = d - dst;
	n = siz - dlen;

	if (n == 0) {
		return(dlen + strlen(s));
	}

	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}

		s++;
	}

	*d = '\0';
	return(dlen + (s - src)); /* count does not include NUL */
}

uint32_t strlcpy(char *dst, const char *src, uint32_t siz)
{
	if (!src || !dst) {
		return 0;
	}

	char *d = dst;
	const char *s = src;
	uint32_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0) {
				break;
			}
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0) {
			*d = '\0'; /* NUL-terminate dst */
		}
		while (*s++);
	}

	return s - src - 1; /* count does not include NUL */
}

char* strdup(const char* string)
{
	if (!string) {
		return NULL;
	}
	uint32_t len = strlen(string);
	char* result = kmalloc(len + 1);
	if (result) {
		strlcpy(result, string, len + 1);
		*(result + len) = 0;
	}
	return result;
}

const char* gc_strdup(basic_ctx* ctx, const char* string)
{
	if (!string || !ctx) {
		return NULL;
	}
	if (!*string) {
		/* For empty strings, the string_gc_storage ptr always starts with a single NULL char */
		return ctx->string_gc_storage;
	}
	uint32_t len = strlen(string);
	if (ctx->string_gc_storage_next + len + 1 > ctx->string_gc_storage + STRING_GC_AREA_SIZE) {
		tokenizer_error_printf(ctx, "Out of string area allocator space storing '%s'", string);
		return NULL;
	}
	const char* result = ctx->string_gc_storage_next;
	memcpy(ctx->string_gc_storage_next, string, len);
	ctx->string_gc_storage_next[len] = '\0';
	ctx->string_gc_storage_next += len + 1;
	return result;
}

int gc(basic_ctx* ctx)
{
	if (ctx->string_gc_storage && ctx->string_gc_storage_next) {
		/* Strings start at +1 as the base ptr is for empties */
		ctx->string_gc_storage_next = ctx->string_gc_storage + 1;
	}
	return 1;
}

static double pow10i(int e) {
	double result = 1.0;
	double base = (e < 0) ? 0.1 : 10.0;

	/* take absolute value of e safely without UB on INT_MIN */
	unsigned int k = (unsigned int)((e < 0) ? -(long long)e : (long long)e);

	while (k > 0u) {
		if ((k & 1u) != 0u) {
			result *= base;
		}
		base *= base;
		k >>= 1;
	}
	return result;
}

/* - No leading whitespace skipped.
 * - No leading '+' or '-' on the mantissa.
 * - Accepts ".123".
 * - Accepts 'e'/'E' with optional '+'/'-' and optional digits (e.g. "1e" == 1.0).
 * - Ignores any trailing junk after the parsed number.
 * - Returns false only for NULL inputs; otherwise true and *a set (default 0.0).
 */
bool atof(const char *s, double *a) {
	if (!s || !a) {
		return false;
	}

	*a = 0.0;
	int exp10 = 0;
	int c;

	/* integer part */
	while ((c = *s++) != '\0' && c >= '0' && c <= '9') {
		*a = (*a) * 10.0 + (double)(c - '0');
	}

	/* fractional part */
	if (c == '.') {
		while ((c = *s++) != '\0' && c >= '0' && c <= '9') {
			*a = (*a) * 10.0 + (double)(c - '0');
			exp10 -= 1;
		}
	}

	/* exponent part (optional; digits also optional, as per original) */
	if (c == 'e' || c == 'E') {
		int sign = 1;
		int iexp = 0;

		c = *s++;
		if (c == '+') {
			c = *s++;
		} else if (c == '-') {
			sign = -1;
			c = *s++;
		}

		while (c >= '0' && c <= '9') {
			iexp = iexp * 10 + (c - '0');
			c = *s++;
		}

		exp10 += iexp * sign;
	}

	if (exp10 != 0) {
		*a *= pow10i(exp10);
	}

	return true;
}

int atoi(const char *s) {
	if (!s) {
		return 0;
	}
	return (int)atoll(s, 10);
}

__attribute__((hot)) int64_t atoll(const char *s, int radix) {
	if (!s) {
		return 0;
	}

	/* Lookup tables: -1 = invalid, otherwise 0..15 */
	static const signed char map10[256] = {
		[0 ... 47] = -1,
		['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
		['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
		[58 ... 255] = -1
	};

	static const signed char map16[256] = {
		[0 ... 47] = -1,
		['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,
		['5'] = 5,  ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,
		[':' ... '@'] = -1,
		['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
		['G' ... '`'] = -1,
		['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
		['g' ... 255] = -1
	};

	const unsigned char *p = (const unsigned char *)s;
	int64_t val = 0;
	char neg = 0;

	/* skip spaces/tabs only, as before */
	while (*p == ' ' || *p == '\t') {
		p++;
	}

	if (radix == 10) {
		if (*p == '-') {
			neg = 1;
			p++;
		} else if (*p == '+') {
			p++;
		}
	} else if (radix == 16) {
		if (*p == '&') {
			p++;
		}
	}

	if (__builtin_expect(radix == 16, 0)) {
		/* Fast hex loop: val = (val << 4) + digit */
		for (;;) {
			signed char d = map16[*p];
			if (__builtin_expect(d < 0, 0)) {
				break;
			}
			val = (val << 4) + d;
			p++;
		}
		return val;
	}

	/* Default (and radix!=16): treat as base 10, matching original */
	for (;;) {
		signed char d = map10[*p];
		if (__builtin_expect(d < 0, 0)) {
			break;
		}
		/* compilers will usually transform *10 into shifts+adds */
		val = val * 10 + d;
		p++;
	}

	if (neg) {
		return -val;
	}
	return val;
}

uint64_t atoull(const char *s) {
	if (!s) {
		return 0;
	}
	/* Call signed version in base 10 and cast up */
	return (uint64_t)atoll(s, 10);
}

size_t strrev(char* s)
{
	if (!s) {
		return 0;
	}
	return memrev(s, strlen(s));
}

int do_atoi(int64_t* dst, char* target, unsigned radix)
{
	if (!dst || !target) {
		return -1;
	}

	int64_t res = 0;
	bool looped = false;
	bool sign = false;
	bool succ = false;
	if (radix < 2 || radix > 36) {
		return -4;
	}
	char* ptr = target;
	char c;
	static const char* alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	while ((c = *ptr++) != 0) {
		if (c == '-') {
			sign = true;
			continue;
		}
		if (strchr("\t\n\v\f\r +_", c) != NULL) {
			continue;
		}
		looped = true;
		char* x = strchr(alphabet, toupper(c));
		if (x == NULL) {
			break;
		}
		size_t pos = x - alphabet;
		if (pos >= radix) {
			break;
		}
		succ = true;
		res = res * radix + pos;
	}
	if (sign) {
		res = -res;
	}
	*dst = res;
	/* Errno 3 is reserved for overflow */
	return !succ
		? looped
			? -2
			: -1
		: 0;
}

int do_itoa(int64_t target, char* buf, unsigned radix)
{
	if (!buf) {
		return -1;
	}

	if (radix < 2 || radix > 36) {
		return -1;
	}
	int sign = 0;
	if (target < 0) {
		sign = 1;
		target = -target;
	}
	char* low = buf;
	do {
		*buf++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[target % radix];
	} while (target /= radix);
	if (sign) {
		*buf++ = '-';
	}
	*buf++ = '\0';
	strrev(low);
	return 0;
}

char* strrchr(const char* s, int c) {
	const char* last = NULL;
	unsigned char ch = (unsigned char)c;

	while (*s != '\0') {
		if ((unsigned char)*s == ch) {
			last = s;
		}
		s++;
	}

	/* Allow search for '\0' itself */
	if (ch == '\0') {
		return (char*)s;
	}

	return (char*)last;
}

int strcasecmp(const char* s1, const char* s2) {
	unsigned char c1, c2;

	while (*s1 != '\0' && *s2 != '\0') {
		c1 = (unsigned char)tolower((unsigned char)*s1);
		c2 = (unsigned char)tolower((unsigned char)*s2);

		if (c1 != c2) {
			return (int)c1 - (int)c2;
		}

		s1++;
		s2++;
	}

	/* Compare the final null terminator as well */
	c1 = (unsigned char)tolower((unsigned char)*s1);
	c2 = (unsigned char)tolower((unsigned char)*s2);
	return (int)c1 - (int)c2;
}

size_t strlen_ansi(const char *s) {
	size_t len = 0;

	while (*s) {
		if (*s == '\x1b' && *(s + 1) == '[') {
			/* Skip CSI sequence */
			s += 2;
			while (*s && !((*s >= '@' && *s <= '~'))) {
				s++;
			}
			if (*s) {
				s++; /* skip final command character */
			}
		} else {
			len++;
			s++;
		}
	}

	return len;
}

/* Minimal strstr implementation */
const char *strstr(const char *haystack, const char *needle)
{
	if (!*needle) {
		return haystack; /* empty needle matches start */
	}

	for (const char *h = haystack; *h; ++h) {
		const char *h_it = h;
		const char *n_it = needle;

		while (*h_it && *n_it && *h_it == *n_it) {
			++h_it;
			++n_it;
		}

		if (*n_it == '\0') {
			return h; /* full match */
		}
	}
	return NULL;
}
