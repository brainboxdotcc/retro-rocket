#include <kernel.h>

struct gc_str* gc_list = NULL;

unsigned int strlen(const char* str)
{
	unsigned int len = 0;
	for(; *str; ++str)
		len++;
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


unsigned char tolower(unsigned char input)
{
	return (input | 0x20);
}

int strcmp(const char* s1, const char* s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return 0;
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

char* strchr(const char *s, int c)
{
	for (; *s; ++s)
	{
		if (*s == c)
			return (char *)s;
	}
	return NULL;
}

int abs(int a)
{
	if (a < 0)
		return +a;
	else
		return a;
}

int labs(s64 a)
{
	if (a < 0)
		return +a;
	else
		return a;
}

int strncmp(const char* s1, const char* s2, u32 n)
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

u64 hextoint(const char* n1)
{
	u32 length = strlen(n1);
	u64 result = 0;
	int i = 0, fact = 1;

	if (length)
	{
		if (length > 16)
			length = 16;

		for(i = length - 1; i >= 0; i--)
		{
			char digit = tolower(*(n1 + i));
			if ((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f'))
			{
				if (digit >= 97)
					result += (digit - 87) * fact;
				else
					result += (digit - 48) * fact;
				fact = fact << 4;
			}
			else
			{
				return 0;
			}
		}
		return result;
	}

	return 0;
}

u32 strlcat(char *dst, const char *src, u32 siz)
{
	char *d = dst;
	const char *s = src;
	u32 n = siz, dlen;

	while (n-- != 0 && *d != '\0')
		d++;

	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));

	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}

		s++;
	}

	*d = '\0';
	return(dlen + (s - src)); /* count does not include NUL */
}

u32 strlcpy(char *dst, const char *src, u32 siz)
{
	char *d = dst;
	const char *s = src;
	u32 n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0)
	{
		do
		{
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0'; /* NUL-terminate dst */
		while (*s++);
	}

	return(s - src - 1); /* count does not include NUL */
}

char* strdup(const char* string)
{
	u32 siz = strlen(string) + 1;
	char* result = (char*)kmalloc(siz);
	strlcpy(result, string, siz);
	*(result+siz) = 0;
	return result;
}

char* gc_strdup(const char* string)
{
	u32 siz = strlen(string) + 1;
	char* result = (char*)kmalloc(siz);
	strlcpy(result, string, siz);
	*(result+siz) = 0;

	if (gc_list == NULL)
	{
		gc_list = (struct gc_str*)kmalloc(sizeof(struct gc_str));
		gc_list->next = NULL;
		gc_list->ptr = result;
	}
	else
	{
		struct gc_str* new = (struct gc_str*)kmalloc(sizeof(struct gc_str));
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
	for (; cur; cur = cur->next)
	{
		n++;
		kfree((char*)cur->ptr);
	}

	for (cur = gc_list; cur; cur = cur->next)
	{
		kfree(cur);
	}

	gc_list = NULL;

	return n;
}



int atoi(const char *s)
{
	static const char digits[] = "0123456789";  /* legal digits in order */
	int val = 0;	 /* value we're accumulating */
	char neg = 0;	      /* set to true if we see a minus sign */

	/* skip whitespace */
	while (*s==' ' || *s=='\t') {
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

s64 atoll(const char *s, int radix)
{
	static const char ddigits[] = "0123456789";  /* legal digits in order */
	static const char xdigits[] = "0123456789ABCDEF";  /* legal digits in order */
	const char* digits = radix == 16 ? xdigits : ddigits;
	s64 val = 0;	 /* value we're accumulating */
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
		s64 digit;
		
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
		s++;
	}
       
	/* handle negative numbers (decimal only) */
	if (neg && radix == 10) {
		return -val;
	}

	//kprintf("atoll(...,%d) = %d\n", radix, val);
       
	/* done */
	return val;
}

u64 atoull(const char *s)
{
	static const char digits[] = "0123456789";  /* legal digits in order */
	u64 val=0;	 /* value we're accumulating */

	/* skip whitespace */
	while (*s==' ' || *s=='\t') {
		s++;
	}

	if (*s=='+') {
		s++;
	}

	/* process each digit */
	while (*s) {
		const char *where;
		u64 digit;
		
		/* look for the digit in the list of digits */
		where = strchr(digits, *s);
		if (where==NULL) {
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
       
	/* done */
	return val;
}
