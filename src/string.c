#include <kernel.h>

gc_str_t* gc_list = NULL;

unsigned int strlen(const char* str)
{
	if (!str || !*str) {
		return 0;
	}
	unsigned int len = 0;
	for(; *str; ++str)
		++len;
	return len;
}

void strtolower(char *s) {
	if (!s) {
		return;
	}
	for (; *s; ++s) {
		*s = tolower(*s);
	}
}

unsigned char isdigit(const char x)
{
	return (x >= '0' && x <= '9');
}

unsigned char isxdigit(const char x)
{
	return (x >= '0' && x <= '9') || (x >= 'A' && x <= 'F');
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

int abs(int a)
{
	uint32_t mask = ~((a >> 31) & 1) + 1;
	return (a ^ mask) - mask;
}

int64_t labs(int64_t a)
{
	uint64_t mask = ~((a >> 63) & 1) + 1;
	return (a ^ mask) - mask;
}

char toupper(char low) {
	return low >= 'a' && low <= 'z' ? low - 32 : low;
}

char tolower(char low) {
	return low >= 'A' && low <= 'Z' ? low + 32 : low;
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

int strncmp(const char* s1, const char* s2, uint32_t n)
{
	if (!s1 || !s2) {
		return (int)s1 - (int)s2;
	}
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
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

int isalnum(const char x)
{
	return ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') || (x >= '0' && x <= '9'));
}

int isupper(const char x)
{
	return (x >= 'A' && x <= 'Z');
}


bool isalpha(const char x)
{
	return (x >= 'A' && x <= 'Z');
}

bool isspace(const char x)
{
	return x == ' ' || x == '\t';
}

char* strdup(const char* string)
{
	if (!string) {
		return NULL;
	}
	uint32_t len = strlen(string);
	char* result = kmalloc(len + 1);
	strlcpy(result, string, len + 1);
	*(result + len) = 0;
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

bool atof(const char* s, double* a)
{
	if (!s || !a) {
		return false;
	}

	*a = 0.0f;
	int e = 0;
	int c;
	while ((c = *s++) != '\0' && isdigit(c)) {
		(*a) = (*a) * 10.0 + (c - '0');
	}
	if (c == '.') {
		while ((c = *s++) != '\0' && isdigit(c)) {
			(*a) = (*a) * 10.0 + (c - '0');
			--e;
		}
	}
	if (c == 'e' || c == 'E') {
		int sign = 1;
		int i = 0;
		c = *s++;
		if (c == '+') {
			c = *s++;
		} else if (c == '-') {
			c = *s++;
			sign = -1;
		}
		while (isdigit(c)) {
			i = i * 10 + (c - '0');
			c = *s++;
		}
		e += i * sign;
	}
	while (e > 0) {
		(*a) *= 10.0;
		--e;
	}
	while (e < 0) {
		(*a) *= 0.1;
		++e;
	}
	return true;
}

int atoi(const char *s)
{
	if (!s) {
		return 0;
	}

	static const char digits[] = "0123456789";  /* legal digits in order */
	int val = 0;	 /* value we're accumulating */
	char neg = 0;	      /* set to true if we see a minus sign */

	/* skip whitespace */
	while (*s == ' ' || *s == '\t') {
		s++;
	}

	/* check for sign */
	if (*s=='-') {
		neg=1;
		s++;
	}
	else if (*s=='+') {
		s++;
	}

	/* process each digit */
	while (*s) {
		const char *where;
		int digit;
		
		/* look for the digit in the list of digits */
		where = strchr(digits, *s);
		if (where == NULL) {
			/* not found; not a digit, so stop */
			break;
		}

		/* get the index into the digit list, which is the value */
		digit = (where - digits);

		/* could (should?) check for overflow here */

		/* shift the number over and add in the new digit */
		val = val*10 + digit;

		/* look at the next character */
		s++;
	}
       
	/* handle negative numbers */
	if (neg) {
		return -val;
	}
       
	/* done */
	return val;
}

int64_t atoll(const char *s, int radix)
{
	if (!s) {
		return 0;
	}

	static const char ddigits[] = "0123456789";  /* legal digits in order */
	static const char xdigits[] = "0123456789ABCDEF";  /* legal digits in order */
	const char* digits = radix == 16 ? xdigits : ddigits;
	int64_t val = 0;	 /* value we're accumulating */
	char neg = 0;	      /* set to true if we see a minus sign */

	/* skip whitespace */
	while (*s == ' ' || *s == '\t') {
		s++;
	}

	/* check for sign */
	if (*s=='-' && radix == 10) {
		neg=1;
		s++;
	} else if (*s=='+' && radix == 10) {
		s++;
	} else if (*s == '&' && radix == 16) {
		s++;
	}

	/* process each digit */
	while (*s) {
		const char *where;
		int64_t digit;
		
		/* look for the digit in the list of digits */
		where = strchr(digits, toupper(*s));
		if (where == NULL) {
			/* not found; not a digit, so stop */
			break;
		}

		/* get the index into the digit list, which is the value */
		digit = (where - digits);

		/* could (should?) check for overflow here */

		/* shift the number over and add in the new digit */
		val = val * radix + digit;

		/* look at the next character */
		++s;
	}
       
	/* handle negative numbers (decimal only) */
	if (neg && radix == 10) {
		return -val;
	}

	//kprintf("atoll(...,%d) = %d\n", radix, val);
       
	/* done */
	return val;
}

uint64_t atoull(const char *s)
{
	if (!s) {
		return 0;
	}

	static const char digits[] = "0123456789";  /* legal digits in order */
	uint64_t val=0;	 /* value we're accumulating */

	/* skip whitespace */
	while (*s == ' ' || *s == '\t') {
		++s;
	}

	if (*s == '+') {
		++s;
	}

	/* process each digit */
	while (*s) {
		const char *where;
		uint64_t digit;
		
		/* look for the digit in the list of digits */
		where = strchr(digits, *s);
		if (where == NULL) {
			/* not found; not a digit, so stop */
			break;
		}

		/* get the index into the digit list, which is the value */
		digit = where - digits;

		/* could (should?) check for overflow here */

		/* shift the number over and add in the new digit */
		val = val * 10 + digit;

		/* look at the next character */
		++s;
	}
       
	/* done */
	return val;
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
