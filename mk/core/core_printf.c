#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>

static void kfformat(void (*)(void *, char), void *, uintmax_t, unsigned, bool);

void
kfvprintf(void (*putc)(void *, char),
	  void (*puts)(void *, const char *, size_t), void *arg, const char *s,
	  va_list ap)
{
	const char *p, *q;
	char ch;
	bool lmod, alt;
	intmax_t val;

	for (p = s; *p != '\0'; p++) {
		if (*p != '%') {
			q = strchr(p + 1, '%');
			if (q == NULL) {
				(*puts)(arg, p, strlen(p));
				break;
			}
			(*puts)(arg, p, q - p);
			p = q;
		}
		lmod = false;
		alt = false;
		val = -1; /* Not necessary, but GCC is still dumb in 2006 XXX */
again:
		switch (*++p) {
		case '#':
			alt++;
			goto again;
		case '%':
			(*putc)(arg, '%');
			break;
		case 'd':
			if (!lmod)
				val = va_arg(ap, signed int);
			kfformat(putc, arg, val, 10, true);
			break;
		case 'l':
			if (!lmod++)
				val = va_arg(ap, long);
			goto again;
		case 'p':
			val = va_arg(ap, uintptr_t);
			(*puts)(arg, "0x", 2);
			kfformat(putc, arg, val, 0x10, false);
			break;
		case 'c':
			ch = (char)va_arg(ap, int);
			(*putc)(arg, ch);
			break;
		case 'j':
			if (!lmod++)
				val = va_arg(ap, intmax_t);
			goto again;
		case 'm':
			val = va_arg(ap, int);
			if (val < 0 || val >= ERROR_COUNT) {
				q = "Unknown, likely invalid error";
			} else {
				q = error_strings[val];
			}
			(*puts)(arg, q, strlen(q));
			break;
		case 'o':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			if (alt && val != 0)
				(*putc)(arg, '0');
			kfformat(putc, arg, val, 010, false);
			break;
		case 's':
			q = va_arg(ap, const char *);
			if (q == NULL)
				q = "(null)";
			(*puts)(arg, q, strlen(q));
			break;
		case 'u':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			kfformat(putc, arg, val, 10, false);
			break;
		case 'x':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			if (alt && val != 0)
				(*puts)(arg, "0x", 2);
			kfformat(putc, arg, val, 0x10, false);
			break;
		case 'z':
			if (!lmod++)
				val = va_arg(ap, ssize_t);
			goto again;
		default:
			(*putc)(arg, '%');
			if (*p != '\0')
				(*putc)(arg, *p);
			break;
		}
	}
}

static void
kfformat(void (*putc)(void *, char), void *arg, uintmax_t val, unsigned base,
	 bool sign)
{
	static const char set[] = "0123456789abcdef";
	char str[sizeof (val) * 8]; /* Worst case is binary, 1 byte per bit.  */
	unsigned i = 0;

	if (sign) {
		sign = 0;
		if ((int64_t)val < 0) {
			(*putc)(arg, '-');
			val = (~val);
			val = val + 1;
		}
	}

	do {
		str[i++] = set[val % base];
		val /= base;
	} while (val != 0);

	while (i--)
		(*putc)(arg, str[i]);
}
