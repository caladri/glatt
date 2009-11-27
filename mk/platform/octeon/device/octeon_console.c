#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <io/console/consoledev.h>

static int
octeon_console_getc(void *sc, char *chp)
{
	return (ERROR_NOT_IMPLEMENTED);
}


static void
octeon_console_putc(void *sc, char ch)
{
	(void)sc;
	(void)ch;
}

static void
octeon_console_puts(void *sc, const char *s, size_t len)
{
	while (len--)
		octeon_console_putc(sc, *s++);
}

static void
octeon_console_flush(void *sc)
{
	(void)sc;
}

static struct console octeon_console = {
	.c_name = "octeon",
	.c_softc = NULL,
	.c_getc = octeon_console_getc,
	.c_putc = octeon_console_putc,
	.c_puts = octeon_console_puts,
	.c_flush = octeon_console_flush,
};

void
platform_console_init(void)
{
	console_init(&octeon_console);
}
