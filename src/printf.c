#include <kernel.h>
#include <string.h>

spinlock printlock = 0;

static int do_printf(const char *fmt, va_list args, fnptr_t fn, void *ptr)
{
	unsigned flags, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state, radix;
	long long num;

	//while (printlock);
	//lock_spinlock(&printlock);

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
			/* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
			__attribute__((fallthrough));
		case 4:
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* xxx - far pointers (%Fp, %Fn) not yet supported */
				__attribute__((fallthrough));
			case 'x':
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				__attribute__((fallthrough));
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
				/* load the value to be printed. l=long=32 bits: */
			DO_NUM:
				if(flags & PR_32)
				{
					if(flags & PR_SG) {
						num = va_arg(args, long long);
					} else {
						num = va_arg(args, unsigned long long);
					}
				}
				/* h=short=16 bits (signed or unsigned) */
				else if(flags & PR_16)
				{
					if(flags & PR_SG)
					{
						num = va_arg(args, int);
					}
					else
					{
						num = va_arg(args, unsigned int);
					}
				}
				/* no h nor l: sizeof(int) bits (signed or unsigned) */
				else
				{
					if(flags & PR_SG)
					{
						num = va_arg(args, int);
					}
					else
					{
						num = va_arg(args, unsigned int);
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
				/* convert binary to octal/decimal/hex ASCII */
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
					int);
				actual_wd = 1;
				goto EMIT2;
			case 's':
				/* disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
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
			EMIT2:
				if((flags & PR_LJ) == 0)
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
			__attribute__((fallthrough));
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	
	//unlock_spinlock(&printlock);
	return count;
}

static int vsprintf_help(unsigned c, void **ptr)
{
	char *dst;

	dst = *ptr;
	*dst++ = (char)c;
	*ptr = dst;
	return 0 ;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int rv;

	rv = do_printf(fmt, args, vsprintf_help, (void *)buf);
	buf[rv] = '\0';
	return rv;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = vsprintf(buf, fmt, args);
	va_end(args);
	return rv;
}

int vprintf_help(unsigned c, [[maybe_unused]] void **ptr)
{
	put(current_console, c);
	return 0;
}

int dvprintf_help(unsigned c, [[maybe_unused]] void **ptr)
{
	dput(c);
	return 0;
}

int vprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, vprintf_help, NULL);
}

int dvprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, dvprintf_help, NULL);
}

int printf(const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = vprintf(fmt, args);
	va_end(args);
	return rv;
}

int dprintf(const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = dvprintf(fmt, args);
	va_end(args);
	return rv;
}

