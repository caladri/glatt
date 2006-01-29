#include <core/types.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>

static struct console *kernel_console;

static void kcformat(uint64_t, unsigned, unsigned);
static void kcputc_noflush(char);

void
console_init(struct console *console)
{
	kernel_console = console;
	kcprintf("Switched to console: %s\n", console->c_name);
}

void
kcputc(char ch)
{
	kcputc_noflush(ch);
	kernel_console->c_flush(kernel_console->c_softc);
}

void
kcputs(const char *s)
{
	while (*s != '\0')
		kcputc_noflush(*s++);
	kernel_console->c_flush(kernel_console->c_softc);
}

void
kcprintf(const char *s, ...)
{
	va_list ap;
	const char *p, *q;
	int lmod, alt;
	long val;

	va_start(ap, s);
	for (p = s; *p != '\0'; p++) {
		if (*p != '%') {
			kcputc_noflush(*p);
			continue;
		}
		lmod = 0;
		alt = 0;
		val = -1; /* Not necessary, but GCC is still dumb in 2006 XXX */
again:
		switch (*++p) {
		case '#':
			alt = 1;
			goto again;
		case '%':
			kcputc_noflush('%');
			break;
		case 'd':
			if (lmod == 0)
				val = va_arg(ap, signed int);
			kcformat(val, 10, 1);
			break;
		case 'l':
			if (lmod++ == 0)
				val = va_arg(ap, long);
			goto again;
		case 's':
			q = va_arg(ap, const char *);
			kcputs(q);
			break;
		case 'u':
			if (lmod == 0)
				val = va_arg(ap, unsigned int);
			kcformat(val, 10, 0);
			break;
		case 'x':
			if (lmod == 0)
				val = va_arg(ap, unsigned int);
			if (alt)
				kcputs("0x");
			kcformat(val, 0x10, 0);
			break;
		default:
			kcputc_noflush('%');
			kcputc_noflush(*p);
			break;
		}
	}
	va_end(ap);
	kernel_console->c_flush(kernel_console->c_softc);
}

static void
kcformat(uint64_t val, unsigned base, unsigned sign)
{
	char set[] = "0123456789abcdef";
	char this;

	if (sign) {
		sign = 0;
		if ((int64_t)val < 0) {
			kcputc_noflush('-');
			val = (~val);
			val = val + 1;
		}
	}
	if (val == 0) {
		kcputc_noflush('0');
		return;
	}
	this = set[val % base];
	if (val / base != 0)
		kcformat(val / base, base, sign);
	kcputc_noflush(this);
}

static void
kcputc_noflush(char ch)
{
	kernel_console->c_putc(kernel_console->c_softc, ch);
}
