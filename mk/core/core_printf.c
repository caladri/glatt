#include <core/types.h>
#include <core/error.h>
#include <core/string.h>

static void kfformat(void (*)(void *, char), void *, uint64_t, unsigned, unsigned);
static void kfputs(void (*)(void *, char), void *, const char *);

void
kfvprintf(void (*put)(void *, char), void *arg, const char *s, va_list ap) 
{
	const char *p, *q;
	char ch;
	bool lmod, alt;
	long val;

	for (p = s; *p != '\0'; p++) {
		if (*p != '%') {
			(*put)(arg, *p);
			continue;
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
			(*put)(arg, '%');
			break;
		case 'd':
			if (!lmod)
				val = va_arg(ap, signed int);
			kfformat(put, arg, val, 10, 1);
			break;
		case 'l':
			if (!lmod++)
				val = va_arg(ap, long);
			goto again;
		case 'p':
			val = va_arg(ap, uintptr_t);
			kfputs(put, arg, "0x");
			kfformat(put, arg, val, 0x10, 0);
			break;
		case 'c':
			ch = (char)va_arg(ap, int);
			(*put)(arg, ch);
			break;
		case 'm':
			val = va_arg(ap, int);
			if (val < 0 || val >= ERROR_COUNT)
				val = ERROR_IT_S_ALRIGHT;
			q = error_strings[val];
			kfputs(put, arg, q);
			break;
		case 's':
			q = va_arg(ap, const char *);
			if (q == NULL)
				q = "(null)";
			kfputs(put, arg, q);
			break;
		case 'u':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			kfformat(put, arg, val, 10, 0);
			break;
		case 'x':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			if (alt)
				kfputs(put, arg, "0x");
			kfformat(put, arg, val, 0x10, 0);
			break;
		case 'z':
			if (!lmod++)
				val = va_arg(ap, ssize_t);
			goto again;
		default:
			(*put)(arg, '%');
			(*put)(arg, *p);
			break;
		}
	}
}

static void
kfformat(void (*put)(void *, char), void *arg, uint64_t val, unsigned base, unsigned sign)
{
	char set[] = "0123456789abcdef";
	char this;

	if (sign) {
		sign = 0;
		if ((int64_t)val < 0) {
			(*put)(arg, '-');
			val = (~val);
			val = val + 1;
		}
	}
	if (val == 0) {
		(*put)(arg, '0');
		return;
	}
	this = set[val % base];
	if (val / base != 0)
		kfformat(put, arg, val / base, base, sign);
	(*put)(arg, this);
}

static void
kfputs(void (*put)(void *, char), void *arg, const char *s)
{
	while (*s != '\0')
		(*put)(arg, *s++);
}
