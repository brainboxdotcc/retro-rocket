#include <kernel.h>

struct gc_str* gc_list = NULL;

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
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return 0;
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

int stricmp(const char* s1, const char* s2)
{
	while (toupper(*s1) == toupper(*s2++))
		if (*s1++ == 0)
			return 0;
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

char* strchr(const char *s, int c)
{
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
	uint32_t len = strlen(string) + 1;
	char* result = kmalloc(len);
	strlcpy(result, string, len);
	*(result + len) = 0;
	return result;
}

char* gc_strdup(const char* string)
{
	uint32_t len = strlen(string) + 1;
	char* result = kmalloc(len);
	strlcpy(result, string, len);
	*(result + len) = 0;

	if (gc_list == NULL) {
		gc_list = kmalloc(sizeof(struct gc_str));
		gc_list->next = NULL;
		gc_list->ptr = result;
	} else {
		struct gc_str* new = kmalloc(sizeof(struct gc_str));
		new->next = gc_list;
		new->ptr = result;
		gc_list = new;
	}

	return result;
}

int gc()
{
	struct gc_str* cur = gc_list;
	int n = 0;
	for (; cur; cur = cur->next) {
		++n;
		kfree(cur->ptr);
		kfree(cur);
	}

	gc_list = NULL;

	return n;
}

bool atof(const char* s, double* a)
{
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
		where = strchr(digits, *s);
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
	return memrev(s, strlen(s));
}

int do_atoi(int64_t* dst, char* target, unsigned radix)
{
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
