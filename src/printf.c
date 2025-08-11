#include <kernel.h>
#include <string.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

typedef enum {
	STATE_NORMAL,     /* awaiting '%' */
	STATE_FLAGS,      /* parsing %-0 */
	STATE_WIDTH,      /* parsing field width */
	STATE_MODIFIERS,  /* parsing l, h, etc */
	STATE_CONVERSION  /* final conversion character */
} printf_state_t;

/* Helper: fetch integer argument according to size/signedness flags */
static inline long long fetch_number(va_list args, unsigned flags)
{
	if (flags & PR_INT64) {
		if (flags & PR_SIGNED) {
			return va_arg(args, long long);
		} else {
			return (long long)va_arg(args, unsigned long long);
		}
	} else if (flags & PR_INT32) {
		if (flags & PR_SIGNED) {
			return va_arg(args, int);
		} else {
			return (long long)va_arg(args, unsigned int);
		}
	} else {
		if (flags & PR_SIGNED) {
			return va_arg(args, int);
		} else {
			return (long long)va_arg(args, unsigned int);
		}
	}
}

/* Helper: emit string with padding/flags */
static size_t emit_string(const unsigned char *s, unsigned flags, unsigned given_wd, fnptr_t fn, void **ptr, const char *end)
{
	size_t count = 0;
	size_t actual_wd = strlen((const char *)s);

	if (flags & PR_WAS_NEG) actual_wd++;

	if ((flags & (PR_WAS_NEG | PR_LEFT_ZEROES)) == (PR_WAS_NEG | PR_LEFT_ZEROES)) {
		if (fn('-', ptr, end)) return count;
		count++;
	}

	if ((flags & PR_LEFT_JUSTIFY) == 0) {
		while (given_wd > actual_wd) {
			if (fn(flags & PR_LEFT_ZEROES ? '0' : ' ', ptr, end)) return count;
			count++;
			given_wd--;
		}
	}

	if ((flags & (PR_WAS_NEG | PR_LEFT_ZEROES)) == PR_WAS_NEG) {
		if (fn('-', ptr, end)) return count;
		count++;
	}

	while (*s != '\0') {
		if (fn(*s++, ptr, end)) return count;
		count++;
	}

	if (given_wd < actual_wd) given_wd = 0;
	else given_wd -= actual_wd;

	while (given_wd--) {
		if (fn(' ', ptr, end)) return count;
		count++;
	}

	return count;
}

/* Helper: emit integer */
static size_t emit_number(long long num, unsigned radix, unsigned flags, unsigned given_wd, fnptr_t fn, void **ptr, const char *end)
{
	unsigned char buf[PR_BUFLEN];
	unsigned char *where = buf + PR_BUFLEN - 1;

	*where = '\0';

	if ((flags & PR_SIGNED) && num < 0) {
		flags |= PR_WAS_NEG;
		num = -num;
	}

	do {
		unsigned long temp = (unsigned long)num % radix;
		*--where = (temp < 10) ? temp + '0'
			: (flags & PR_UPPER_HEX) ? temp - 10 + 'A' : temp - 10 + 'a';
		num = (unsigned long)num / radix;
	} while (num != 0);

	return emit_string(where, flags, given_wd, fn, ptr, end);
}

/* Core formatter */
static int do_printf(const char *fmt, size_t max, va_list args, fnptr_t fn, void *ptr)
{
	unsigned flags = 0, given_wd = 0, count = 0;
	printf_state_t state = STATE_NORMAL;
	const char* end = (max == SIZE_MAX ? (const char*)SIZE_MAX : (const char*)ptr + max);

	for (; *fmt; fmt++) {
		switch (state) {
			case STATE_NORMAL:
				if (*fmt != '%') {
					if (fn(*fmt, &ptr, end)) return count;
					count++;
					break;
				}
				state = STATE_FLAGS;
				continue;

			case STATE_FLAGS:
				if (*fmt == '%') {
					if (fn('%', &ptr, end)) return count;
					count++;
					state = STATE_NORMAL;
					flags = given_wd = 0;
					break;
				}
				if (*fmt == '-') { flags |= PR_LEFT_JUSTIFY; break; }
				if (*fmt == '0') { flags |= PR_LEFT_ZEROES; break; }
				state = STATE_WIDTH;
				[[fallthrough]];

			case STATE_WIDTH:
				if (*fmt >= '0' && *fmt <= '9') {
					given_wd = 10 * given_wd + (*fmt - '0');
					break;
				}
				state = STATE_MODIFIERS;
				[[fallthrough]];

			case STATE_MODIFIERS:
				if (*fmt == 'l' || *fmt == 'z') {
					flags |= PR_INT64;
					break;
				}
				if (*fmt == 'h') {
					flags |= PR_INT32;
					break;
				}
				state = STATE_CONVERSION;
				[[fallthrough]];

			case STATE_CONVERSION: {
				switch (*fmt) {
					case 'X': flags |= PR_UPPER_HEX;
						count += emit_number(fetch_number(args, flags), 16, flags, given_wd, fn, &ptr, end);
						break;
					case 'x': case 'p': case 'n':
						count += emit_number(fetch_number(args, flags), 16, flags, given_wd, fn, &ptr, end);
						break;
					case 'd': case 'i':
						flags |= PR_SIGNED;
						count += emit_number(fetch_number(args, flags), 10, flags, given_wd, fn, &ptr, end);
						break;
					case 'u':
						count += emit_number(fetch_number(args, flags), 10, flags, given_wd, fn, &ptr, end);
						break;
					case 'o':
						count += emit_number(fetch_number(args, flags), 8, flags, given_wd, fn, &ptr, end);
						break;
					case 'c': {
						unsigned char cbuf[2] = { (unsigned char)va_arg(args, int), 0 };
						flags &= ~PR_LEFT_ZEROES;
						count += emit_string(cbuf, flags, given_wd, fn, &ptr, end);
						break;
					}
					case 's': {
						unsigned char *s = va_arg(args, unsigned char *);
						flags &= ~PR_LEFT_ZEROES;
						count += emit_string(s, flags, given_wd, fn, &ptr, end);
						break;
					}
					default:
						break;
				}
				state = STATE_NORMAL;
				flags = given_wd = 0;
				break;
			}
		}
	}
	return count;
}

/* Output helpers */
static int vsprintf_help(unsigned c, void **ptr, const void* max)
{
	char *dst = *ptr;
	const char* m = max;
	if (dst < m - 1) {
		*dst++ = (char)c;
		*ptr = dst;
		return 0;
	}
	*dst++ = 0;
	return 1;
}

int vsnprintf(char *buf, size_t max, const char *fmt, va_list args)
{
	int rv = do_printf(fmt, max, args, vsprintf_help, (void *)buf);
	buf[rv] = '\0';
	return rv;
}

int snprintf(char *buf, size_t max, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int rv = vsnprintf(buf, max, fmt, args);
	va_end(args);
	return rv;
}

int vprintf_help(unsigned c, [[maybe_unused]] void **ptr, [[maybe_unused]] const void* max)
{
	put(current_console, c);
	return 0;
}

static char dprintf_format_buffer[MAX_STRINGLEN] = { 0 };
static char* dprintf_buf_offset = dprintf_format_buffer;

static void append_debug_buf(char x) {
	if (dprintf_format_buffer >= dprintf_format_buffer && dprintf_buf_offset - dprintf_format_buffer < MAX_STRINGLEN - 1) {
		*dprintf_buf_offset++ = x;
	}
}

int dvprintf_help(unsigned c, [[maybe_unused]] void **ptr, [[maybe_unused]] const void* max)
{
	dput(c);
	//append_debug_buf(c);
	return 0;
}

int vprintf(const char *fmt, va_list args)
{
	uint64_t flags;
	lock_spinlock_irq(&console_spinlock, &flags);
	lock_spinlock(&debug_console_spinlock);
	int r = do_printf(fmt, SIZE_MAX, args, vprintf_help, NULL);
	unlock_spinlock(&debug_console_spinlock);
	unlock_spinlock_irq(&console_spinlock, flags);
	return r;
}

int dvprintf(const char *fmt, va_list args)
{

	char counter[25];
	uint64_t flags;
	lock_spinlock_irq(&debug_console_spinlock, &flags);
	//dprintf_buf_offset = dprintf_format_buffer;
	//memset(dprintf_format_buffer, 0, sizeof(dprintf_format_buffer));
	do_itoa(get_ticks(), counter, 10);
	//append_debug_buf('[');
	dput('[');
	dputstring(counter);
	//for (char* x = counter; *x; ++x) {
	//	append_debug_buf(*x);
	//}
	dputstring("]: ");
	//append_debug_buf(']');
	//append_debug_buf(':');
	//append_debug_buf(' ');
	int r = do_printf(fmt, MAX_STRINGLEN, args, dvprintf_help, NULL);
	//append_debug_buf(0);
	//dprintf_buffer_append_line(dprintf_format_buffer, (uint64_t)dprintf_buf_offset - (uint64_t)&dprintf_format_buffer);
	unlock_spinlock_irq(&debug_console_spinlock, flags);
	return r;
}

int printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int rv = vprintf(fmt, args);
	va_end(args);
	return rv;
}

int dprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int rv = dvprintf(fmt, args);
	va_end(args);
	return rv;
}
