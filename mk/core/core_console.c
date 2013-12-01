#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/sleepq.h>
#include <core/string.h>
#include <core/console.h>
#include <core/consoledev.h>

static struct sleepq kernel_input_sleepq;
static struct console *kernel_input, *kernel_output;

static void cflush(struct console *);
static void cputc_noflush(void *, char);
static void cputs_noflush(void *, const char *, size_t);

#define	CONSOLE_LOCK(c)		spinlock_lock(&(c)->c_lock)
#define	CONSOLE_UNLOCK(c)	spinlock_unlock(&(c)->c_lock)

void
console_init(struct console *console)
{
	spinlock_init(&console->c_lock, "console", SPINLOCK_FLAG_DEFAULT);

	if (console->c_putc != NULL) {
		kernel_output = console;
		printf("Switched to output console: %s\n", console->c_name);
	}

	if (console->c_getc != NULL) {
		/* XXX Push sleepq into console?  */
		if (kernel_input == NULL)
			sleepq_init(&kernel_input_sleepq, &console->c_lock);
		else
			kernel_input_sleepq.sq_lock = &console->c_lock; /* XXX Filthy.  */

		kernel_input = console;
		printf("Switched to input console: %s\n", console->c_name);
	}
}

int
kcgetc(char *chp)
{
	char ch;
	int error;

	CONSOLE_LOCK(kernel_input);
	error = kernel_input->c_getc(kernel_input->c_softc, &ch);
	CONSOLE_UNLOCK(kernel_input);
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
		CONSOLE_LOCK(kernel_input);
		error = kernel_input->c_getc(kernel_input->c_softc, &ch);
		switch (error) {
		case ERROR_AGAIN:
			sleepq_enter(&kernel_input_sleepq);
			/* sleepq_enter drops console lock.  */
			continue;
		default:
			CONSOLE_UNLOCK(kernel_input);
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
	CONSOLE_LOCK(kernel_input);
	if (broadcast)
		sleepq_signal(&kernel_input_sleepq);
	else
		sleepq_signal_one(&kernel_input_sleepq);
	CONSOLE_UNLOCK(kernel_input);
}

void
kcputc(char ch)
{
	CONSOLE_LOCK(kernel_output);
	cputc_noflush(kernel_output, ch);
	cflush(kernel_output);
	CONSOLE_UNLOCK(kernel_output);
}

void
kcputs(const char *s)
{
	CONSOLE_LOCK(kernel_output);
	cputs_noflush(kernel_output, s, strlen(s));
	cflush(kernel_output);
	CONSOLE_UNLOCK(kernel_output);
}

void
kcputsn(const char *s, size_t len)
{
	CONSOLE_LOCK(kernel_output);
	cputs_noflush(kernel_output, s, len);
	cflush(kernel_output);
	CONSOLE_UNLOCK(kernel_output);
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
	CONSOLE_LOCK(kernel_output);
	kfvprintf(cputc_noflush, cputs_noflush, kernel_output, s, ap);
	cflush(kernel_output);
	CONSOLE_UNLOCK(kernel_output);
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
