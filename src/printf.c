#include <kernel.h>
#include <string.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

static int do_printf(const char *fmt, size_t max, va_list args, fnptr_t fn, void *ptr)
{
	unsigned flags = 0, actual_wd = 0, count = 0, given_wd = 0;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state = 0, radix = 10;
	long long num = 0;
	const char* end = (max == SIZE_MAX ? (const char*)SIZE_MAX : ptr + max);

	/* begin scanning format specifier list */
	for(; *fmt; fmt++) {
		switch(state) {
			/* STATE 0: AWAITING % */
			case 0:
				if(*fmt != '%')	{/* not %... */
					if (fn(*fmt, &ptr, end)) return count;	/* ...just echo it */
					count++;
					break;
				}
				/* found %, get next char and advance state to check if next char is a flag */
				state++;
				fmt++;
				/* FALL THROUGH */
				/* STATE 1: AWAITING FLAGS (%-0) */
			case 1:
				if(*fmt == '%')	{
					if (fn(*fmt, &ptr, end)) return count;
					count++;
					state = flags = given_wd = 0;
					break;
				}
				if(*fmt == '-') {
					if(flags & PR_LJ)/* %-- is illegal */
						state = flags = given_wd = 0;
					else
						flags |= PR_LJ;
					break;
				}
				/* not a flag char: advance state to check if it's field width */
				state++;
				/* check now for '%0...' */
				if(*fmt == '0') {
					flags |= PR_LZ;
					fmt++;
				}
				/* FALL THROUGH */
				/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
			case 2:
				if(*fmt >= '0' && *fmt <= '9') {
					given_wd = 10 * given_wd +
						(*fmt - '0');
					break;
				}
				/* not field width: advance state to check if it's a modifier */
				state++;
				/* FALL THROUGH */
				/* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
			case 3:
				if(*fmt == 'F') {
					flags |= PR_FP;
					break;
				}
				if(*fmt == 'N') {
					break;
				}
				if(*fmt == 'l') {
					flags |= PR_32;
					break;
				}
				if(*fmt == 'h') {
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
				switch(*fmt) {
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
					if(flags & PR_32) {
						if(flags & PR_SG) {
							num = va_arg(args, long long);
						} else {
							num = va_arg(args, unsigned long long);
						}
					} else if(flags & PR_16) {
						/* h=short=16 bits (signed or unsigned) */
						if(flags & PR_SG) {
							num = va_arg(args, int);
						} else {
							num = va_arg(args, unsigned int);
						}
					} else {
						/* no h nor l: sizeof(int) bits (signed or unsigned) */
						if(flags & PR_SG) {
							num = va_arg(args, int);
						} else {
							num = va_arg(args, unsigned int);
						}
					}
					/* take care of sign */
					if(flags & PR_SG) {
						if(num < 0) {
							flags |= PR_WS;
							num = -num;
						}
					}
					/* convert binary to octal/decimal/hex ASCII */
					do {
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
					} while(num != 0);
					goto EMIT;
				case 'c':
					/* disallow pad-left-with-zeroes for %c */
					flags &= ~PR_LZ;
					where--;
					*where = (unsigned char)va_arg(args, int);
					actual_wd = 1;
					goto EMIT2;
				case 's':
					/* disallow pad-left-with-zeroes for %s */
					flags &= ~PR_LZ;
					where = va_arg(args, unsigned char *);
				EMIT:
					actual_wd = strlen((const char*)where);
					if (flags & PR_WS)
						actual_wd++;
					/* if we pad left with ZEROES, do the sign now */
					if ((flags & (PR_WS | PR_LZ)) == (PR_WS | PR_LZ)) {
						if (fn('-', &ptr, end)) return count;
						count++;
					}
					/* pad on left with spaces or zeroes (for right justify) */
				EMIT2:
					if ((flags & PR_LJ) == 0) {
						while(given_wd > actual_wd) {
							if (fn(flags & PR_LZ ? '0' : ' ', &ptr, end)) return count;
							count++;
							given_wd--;
						}
					}
					/* if we pad left with SPACES, do the sign now */
					if ((flags & (PR_WS | PR_LZ)) == PR_WS) {
						if (fn('-', &ptr, end)) return count;
						count++;
					}
					/* emit string/char/converted number */
					while (*where != '\0') {
						if (fn(*where++, &ptr, end)) return count;
						count++;
					}
					/* pad on right with spaces (for left justify) */
					if (given_wd < actual_wd)
						given_wd = 0;
					else
						given_wd -= actual_wd;
					for (; given_wd; given_wd--) {
						if (fn(' ', &ptr, end)) return count;
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
	return count;
}

static int vsprintf_help(unsigned c, void **ptr, const void* max)
{
	char *dst = *ptr;
	const char* m = max;
	if (dst < m - 1) {
		*dst++ = (char)c;
		*ptr = dst;
		return 0;
	}
	/* Reached maximum of buffer, terminate string and exit
	 * with error state
	 */
	*dst++ = 0;
	return 1;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int rv;

	rv = do_printf(fmt, SIZE_MAX, args, vsprintf_help, (void *)buf);
	buf[rv] = '\0';
	return rv;
}

int vsnprintf(char *buf, size_t max, const char *fmt, va_list args)
{
	int rv;

	rv = do_printf(fmt, max, args, vsprintf_help, (void *)buf);
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

int snprintf(char *buf, size_t max, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = vsnprintf(buf, max, fmt, args);
	va_end(args);
	return rv;
}


int vprintf_help(unsigned c, [[maybe_unused]] void **ptr, [[maybe_unused]] const void* max)
{
	put(current_console, c);
	return 0;
}

int dvprintf_help(unsigned c, [[maybe_unused]] void **ptr, [[maybe_unused]] const void* max)
{
	dput(c);
	return 0;
}

int vprintf(const char *fmt, va_list args)
{
	lock_spinlock(&console_spinlock);
	int r = do_printf(fmt, SIZE_MAX, args, vprintf_help, NULL);
	unlock_spinlock(&console_spinlock);
	return r;
}

int dvprintf(const char *fmt, va_list args)
{
	char counter[25];
	lock_spinlock(&debug_console_spinlock);
	do_itoa(get_ticks(), counter, 10);
	dput('[');
	dputstring(counter);
	dputstring("]: ");
	int r = do_printf(fmt, SIZE_MAX, args, dvprintf_help, NULL);
	unlock_spinlock(&debug_console_spinlock);
	return r;
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

