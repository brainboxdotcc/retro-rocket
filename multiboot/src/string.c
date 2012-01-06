#include "../include/string.h"
#include "../include/kmalloc.h"

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

int strncmp(const char* s1, const char* s2, u32int n)
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

u32int hextoint(const char* n1)
{
	u32int length = strlen(n1);
	u32int result = 0;
	int i = 0, fact = 1;

	if (length)
	{
		if (length>8)
			length = 8;

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

u32int strlcat(char *dst, const char *src, u32int siz)
{
	char *d = dst;
	const char *s = src;
	u32int n = siz, dlen;

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

u32int strlcpy(char *dst, const char *src, u32int siz)
{
	char *d = dst;
	const char *s = src;
	u32int n = siz;

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
	u32int siz = strlen(string) + 1;
	char* result = (char*)kmalloc(siz);
	strlcpy(result, string, siz);
	*(result+siz) = 0;
	return result;
}

char* gc_strdup(const char* string)
{
	u32int siz = strlen(string) + 1;
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
	for (; cur; cur = cur->next)
	{
		kfree(cur->ptr);
	}

	for (cur = gc_list; cur; cur = cur->next)
	{
		kfree(cur);
	}

	gc_list = NULL;
}



int atoi(const char *s)
{
        static const char digits[] = "0123456789";  /* legal digits in order */
        unsigned val=0;         /* value we're accumulating */
        int neg=0;              /* set to true if we see a minus sign */

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
                unsigned digit;
                
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
       
        /* handle negative numbers */
        if (neg) {
                return -val;
        }
       
        /* done */
        return val;
}
