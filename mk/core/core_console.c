#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/console.h>
#include <core/consoledev.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <vm/vm_page.h>

static struct sleepq kernel_input_sleepq;
static struct console *kernel_input, *kernel_output;

static void cflush(struct console *);
static void cputc_noflush(void *, char);
static void cputs_noflush(void *, const char *, size_t);

static int console_handler(void *, struct ipc_header *, void *);
static void console_startup(void *);

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

static int
console_handler(void *arg, struct ipc_header *reqh, void *p)
{
	struct ipc_header ipch;
	int error;
	char ch;

	switch (reqh->ipchdr_msg) {
	case CONSOLE_MSG_PUTC:
		if (p != NULL)
			return (ERROR_INVALID);
		kcputc(reqh->ipchdr_param);
		return (0);
	case CONSOLE_MSG_PUTS:
		if (p == NULL || reqh->ipchdr_param > PAGE_SIZE)
			return (ERROR_INVALID);
		kcputsn(p, reqh->ipchdr_param);
		return (0);
	case CONSOLE_MSG_GETC:
		error = kcgetc_wait(&ch);
		if (error != 0) {
			ipch = IPC_HEADER_ERROR(reqh, error);
		} else {
			ipch = IPC_HEADER_REPLY(reqh);
			ipch.ipchdr_param = ch;
		}

		error = ipc_port_send_data(&ipch, NULL, 0);
		if (error != 0) {
			printf("%s: ipc_port_send failed: %m\n", __func__, error);
			return (error);
		}
		return (0);
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static void
console_startup(void *arg)
{
	int error;

	error = ipc_service("console", IPC_PORT_CONSOLE, IPC_PORT_FLAG_PUBLIC | IPC_PORT_FLAG_NEW,
			    console_handler, NULL);
	if (error != 0)
		panic("%s: ipc_service failed: %m", __func__, error);
}
STARTUP_ITEM(console, STARTUP_SERVERS, STARTUP_FIRST, console_startup, NULL);
