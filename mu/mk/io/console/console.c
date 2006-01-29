#include <core/types.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>

static struct console *kernel_console;

static void kcputc_noflush(char ch);

void
console_init(struct console *console)
{
	kernel_console = console;
	kcputs("Switched to console: ");
	kcputs(console->c_name);
	kcputc('\n');
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
	kcputs("kcprintf called, but unsupported... format string: ");
	kcputs(s);
}

static void
kcputc_noflush(char ch)
{
	kernel_console->c_putc(kernel_console->c_softc, ch);
}
