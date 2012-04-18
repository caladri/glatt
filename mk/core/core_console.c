#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/sleepq.h>
#include <core/string.h>
#include <core/console.h>
#include <core/consoledev.h>

static struct console *kernel_console;

static void cflush(struct console *);
static void cputc_noflush(void *, char);
static void cputs_noflush(void *, const char *, size_t);

#define	CONSOLE_LOCK(c)		spinlock_lock(&(c)->c_lock)
#define	CONSOLE_UNLOCK(c)	spinlock_unlock(&(c)->c_lock)

void
console_init(struct console *console)
{
	spinlock_init(&console->c_lock, "console", SPINLOCK_FLAG_DEFAULT);
	kernel_console = console;
	printf("Switched to console: %s\n", console->c_name);
}

int
kcgetc(char *chp)
{
	char ch;
	int error;

	CONSOLE_LOCK(kernel_console);
	error = kernel_console->c_getc(kernel_console->c_softc, &ch);
	CONSOLE_UNLOCK(kernel_console);
	if (error != 0)
		return (error);
	*chp = ch;
	return (0);
}

int
kcgetc_wait(char *chp)
{
	char ch;
	int error;

	for (;;) {
		CONSOLE_LOCK(kernel_console);
		error = kernel_console->c_getc(kernel_console->c_softc, &ch);
		switch (error) {
		case ERROR_AGAIN:
			sleepq_enter(kernel_console, &kernel_console->c_lock);
			/* sleepq_enter drops console lock.  */
			continue;
		default:
			CONSOLE_UNLOCK(kernel_console);
			if (error != 0)
				return (error);
			*chp = ch;
			return (0);
		}
	}
}

void
kcgetc_wakeup(bool broadcast)
{
	if (broadcast)
		sleepq_signal(kernel_console);
	else
		sleepq_signal_one(kernel_console);
}

void
kcputc(char ch)
{
	CONSOLE_LOCK(kernel_console);
	cputc_noflush(kernel_console, ch);
	cflush(kernel_console);
	CONSOLE_UNLOCK(kernel_console);
}

void
kcputs(const char *s)
{
	CONSOLE_LOCK(kernel_console);
	cputs_noflush(kernel_console, s, strlen(s));
	cflush(kernel_console);
	CONSOLE_UNLOCK(kernel_console);
}

void
kcputsn(const char *s, size_t len)
{
	CONSOLE_LOCK(kernel_console);
	cputs_noflush(kernel_console, s, len);
	cflush(kernel_console);
	CONSOLE_UNLOCK(kernel_console);
}

void
printf(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vprintf(s, ap);
	va_end(ap);
}

void
vprintf(const char *s, va_list ap)
{
	CONSOLE_LOCK(kernel_console);
	kfvprintf(cputc_noflush, cputs_noflush, kernel_console, s, ap);
	cflush(kernel_console);
	CONSOLE_UNLOCK(kernel_console);
}

static void
cflush(struct console *console)
{
	if (console->c_flush != NULL)
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
cputs_noflush(void *arg, const char *s, size_t len)
{
	struct console *console;

	console = arg;

	if (console->c_puts == NULL) {
		while (len--)
			cputc_noflush(console, *s++);
		return;
	}
	console->c_puts(console->c_softc, s, len);
}
