#include <core/types.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>

static struct console *kernel_console;

static void kcformat(uint64_t, unsigned, unsigned);
static void kcflush(void);
static void kcputc_noflush(char);
static void kcputs_noflush(const char *);

void
console_init(struct console *console)
{
	spinlock_init(&console->c_lock, "console");
	kernel_console = console;
	kcprintf("Switched to console: %s\n", console->c_name);
}

void
kcputc(char ch)
{
	spinlock_lock(&kernel_console->c_lock);
	kcputc_noflush(ch);
	kcflush();
	spinlock_unlock(&kernel_console->c_lock);
}

void
kcputs(const char *s)
{
	spinlock_lock(&kernel_console->c_lock);
	kcputs_noflush(s);
	kcflush();
	spinlock_unlock(&kernel_console->c_lock);
}

void
kcprintf(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
}

void
kcvprintf(const char *s, va_list ap) 
{
	const char *p, *q;
	bool lmod, alt;
	long val;

	spinlock_lock(&kernel_console->c_lock);
	for (p = s; *p != '\0'; p++) {
		if (*p != '%') {
			kcputc_noflush(*p);
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
			kcputc_noflush('%');
			break;
		case 'd':
			if (!lmod)
				val = va_arg(ap, signed int);
			kcformat(val, 10, 1);
			break;
		case 'l':
			if (!lmod++)
				val = va_arg(ap, long);
			goto again;
		case 'p':
			val = va_arg(ap, uintptr_t);
			kcputs_noflush("0x");
			kcformat(val, 0x10, 0);
			break;
		case 's':
			q = va_arg(ap, const char *);
			kcputs_noflush(q);
			break;
		case 'u':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			kcformat(val, 10, 0);
			break;
		case 'x':
			if (!lmod)
				val = va_arg(ap, unsigned int);
			if (alt)
				kcputs_noflush("0x");
			kcformat(val, 0x10, 0);
			break;
		default:
			kcputc_noflush('%');
			kcputc_noflush(*p);
			break;
		}
	}
	kcflush();
	spinlock_unlock(&kernel_console->c_lock);
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
kcflush(void)
{
	kernel_console->c_flush(kernel_console->c_softc);
}

static void
kcputc_noflush(char ch)
{
	kernel_console->c_putc(kernel_console->c_softc, ch);
}

static void
kcputs_noflush(const char *s)
{
	while (*s != '\0')
		kcputc_noflush(*s++);
}
