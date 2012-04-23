#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

static void printf_putc(void *, char);
static void printf_puts(void *, const char *, size_t);

void
puts(const char *s)
{
	putsn(s, strlen(s));
}

int
getline(char *buf, size_t len)
{
	unsigned i;
	int ch;

	for (i = 0; i < len; i++) {
again:		ch = getchar();
		if (ch == -1)
			return (ERROR_UNEXPECTED);
		if (ch == '\010') {
			if (i != 0) {
				putchar(ch);
				putchar(' ');
				putchar(ch);
				i--;
			}
			goto again;
		}
		putchar(ch); /* Echo back.  */
		if (ch == '\n') {
			buf[i] = '\0';
			return (0);
		}
		buf[i] = ch;
	}

	return (ERROR_FULL);
}

void
printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
vprintf(const char *fmt, va_list ap)
{
	kfvprintf(printf_putc, printf_puts, NULL, fmt, ap);
}


static void
printf_putc(void *arg, char ch)
{
	(void)arg;
	putchar(ch);
}

static void
printf_puts(void *arg, const char *s, size_t len)
{
	(void)arg;

	putsn(s, len);
}
