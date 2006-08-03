#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>

static struct console *kernel_console;

static void cflush(struct console *);
static void cputc_noflush(void *, char);
static void cputs_noflush(struct console *, const char *);

void
console_init(struct console *console)
{
	spinlock_init(&console->c_lock, "console");
	kernel_console = console;
	kcprintf("Switched to console: %s\n", console->c_name);
}

int
kcgetc(char *chp)
{
	char ch;
	int error;

	spinlock_lock(&kernel_console->c_lock);
	error = kernel_console->c_getc(kernel_console->c_softc, &ch);
	spinlock_unlock(&kernel_console->c_lock);
	if (error != 0)
		return (error);
	*chp = ch;
	return (0);
}

void
kcputc(char ch)
{
	spinlock_lock(&kernel_console->c_lock);
	cputc_noflush(kernel_console, ch);
	cflush(kernel_console);
	spinlock_unlock(&kernel_console->c_lock);
}

void
kcputs(const char *s)
{
	spinlock_lock(&kernel_console->c_lock);
	cputs_noflush(kernel_console, s);
	cflush(kernel_console);
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
	spinlock_lock(&kernel_console->c_lock);
	kfvprintf(cputc_noflush, kernel_console, s, ap);
	cflush(kernel_console);
	spinlock_unlock(&kernel_console->c_lock);
}

static void
cflush(struct console *console)
{
	console->c_flush(console->c_softc);
}

static void
cputc_noflush(void *arg, char ch)
{
	struct console *console;

	console = arg;

	console->c_putc(console->c_softc, ch);
	if (ch == '\n')
		cflush(console);
}

static void
cputs_noflush(struct console *console, const char *s)
{
	while (*s != '\0')
		cputc_noflush(console, *s++);
}
