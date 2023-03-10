#include <video.h>
#include <kernel.h>
#include <string.h>
#include <kprintf.h>

/*****************************************************************************
name:	do_printf
action:	minimal subfunction for ?printf, calls function
	'fn' with arg 'ptr' for each character to be output
returns:total number of characters output
*****************************************************************************/
static int do_printf(const char *fmt, va_list args, fnptr_t fn, void *ptr)
{
	unsigned flags, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state, radix;
	long num;

	/* FIX: Skip the first argument in the va_list, as it is the format specifier! */
	va_incr(args, const char*);

	state = flags = count = given_wd = 0;
/* begin scanning format specifier list */
	for(; *fmt; fmt++)
	{
		switch(state)
		{
/* STATE 0: AWAITING % */
		case 0:
			if(*fmt != '%')	/* not %... */
			{
				fn(*fmt, &ptr);	/* ...just echo it */
				count++;
				break;
			}
/* found %, get next char and advance state to check if next char is a flag */
			state++;
			fmt++;
			/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS (%-0) */
		case 1:
			if(*fmt == '%')	/* %% */
			{
				fn(*fmt, &ptr);
				count++;
				state = flags = given_wd = 0;
				break;
			}
			if(*fmt == '-')
			{
				if(flags & PR_LJ)/* %-- is illegal */
					state = flags = given_wd = 0;
				else
					flags |= PR_LJ;
				break;
			}
/* not a flag char: advance state to check if it's field width */
			state++;
/* check now for '%0...' */
			if(*fmt == '0')
			{
				flags |= PR_LZ;
				fmt++;
			}
			/* FALL THROUGH */
/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
		case 2:
			if(*fmt >= '0' && *fmt <= '9')
			{
				given_wd = 10 * given_wd +
					(*fmt - '0');
				break;
			}
/* not field width: advance state to check if it's a modifier */
			state++;
			/* FALL THROUGH */
/* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
		case 3:
			if(*fmt == 'F')
			{
				flags |= PR_FP;
				break;
			}
			if(*fmt == 'N')
				break;
			if(*fmt == 'l')
			{
				flags |= PR_32;
				break;
			}
			if(*fmt == 'h')
			{
				flags |= PR_16;
				break;
			}
/* not modifier: advance state to check if it's a conversion char */
			state++;
			/* FALL THROUGH */
/* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
		case 4:
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* FALL THROUGH */
/* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'x':
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				/* FALL THROUGH */
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
/* load the value to be printed. l=long=32 bits: */
DO_NUM:				if(flags & PR_32)
				{
					num = va_arg(args, unsigned long);
					va_incr(args, unsigned long);
				}
/* h=short=16 bits (signed or unsigned) */
				else if(flags & PR_16)
				{
					if(flags & PR_SG)
					{
						num = va_arg(args, short);
						va_incr(args, unsigned short);
					}
					else
					{
						num = va_arg(args, unsigned short);
						va_incr(args, unsigned short);
					}
				}
/* no h nor l: sizeof(int) bits (signed or unsigned) */
				else
				{
					if(flags & PR_SG)
					{
						num = va_arg(args, int);
						va_incr(args, int);
					}
					else
					{
						num = va_arg(args, unsigned int);
						va_incr(args, unsigned int);
					}
				}
/* take care of sign */
				if(flags & PR_SG)
				{
					if(num < 0)
					{
						flags |= PR_WS;
						num = -num;
					}
				}
/* convert binary to octal/decimal/hex ASCII
OK, I found my mistake. The math here is _always_ unsigned */
				do
				{
					unsigned long temp;

					temp = (unsigned long)num % radix;
					where--;
					if(temp < 10)
						*where = temp + '0';
					else if(flags & PR_CA)
						*where = temp - 10 + 'A';
					else
						*where = temp - 10 + 'a';
					num = (unsigned long)num / radix;
				}
				while(num != 0);
				goto EMIT;
			case 'c':
/* disallow pad-left-with-zeroes for %c */
				flags &= ~PR_LZ;
				where--;
				*where = (unsigned char)va_arg(args,
					unsigned char);
				va_incr(args, unsigned char);
				actual_wd = 1;
				goto EMIT2;
			case 's':
/* disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
				va_incr(args, unsigned char*);
EMIT:
				actual_wd = strlen((const char*)where);
				if(flags & PR_WS)
					actual_wd++;
/* if we pad left with ZEROES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) ==
					(PR_WS | PR_LZ))
				{
					fn('-', &ptr);
					count++;
				}
/* pad on left with spaces or zeroes (for right justify) */
EMIT2:				if((flags & PR_LJ) == 0)
				{
					while(given_wd > actual_wd)
					{
						fn(flags & PR_LZ ?
							'0' : ' ', &ptr);
						count++;
						given_wd--;
					}
				}
/* if we pad left with SPACES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) == PR_WS)
				{
					fn('-', &ptr);
					count++;
				}
/* emit string/char/converted number */
				while(*where != '\0')
				{
					fn(*where++, &ptr);
					count++;
				}
/* pad on right with spaces (for left justify) */
				if(given_wd < actual_wd)
					given_wd = 0;
				else given_wd -= actual_wd;
				for(; given_wd; given_wd--)
				{
					fn(' ', &ptr);
					count++;
				}
				break;
			default:
				break;
			}
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	return count;
}

/*****************************************************************************
SPRINTF
*****************************************************************************/
static int kvsprintf_help(unsigned c, void **ptr)
{
	char *dst;

	dst = *ptr;
	*dst++ = (char)c;
	*ptr = dst;
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int kvsprintf(char *buf, const char *fmt, va_list args)
{
	int rv;

	rv = do_printf(fmt, args, kvsprintf_help, (void *)buf);
	buf[rv] = '\0';
	return rv;
}
/*****************************************************************************
*****************************************************************************/
int ksprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = kvsprintf(buf, fmt, args);
	va_end(args);
	return rv;
}
/*****************************************************************************
*****************************************************************************/
int kvprintf_help(unsigned c, void **ptr)
{
	put(current_console, c);
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int kvprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, kvprintf_help, NULL);
}
/*****************************************************************************
*****************************************************************************/
int kprintf(const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = kvprintf(fmt, args);
	va_end(args);
	//blitconsole(current_console);
	return rv;
}
/*****************************************************************************
*****************************************************************************/

