#include <core/types.h>
#include <core/error.h>
#include <io/console/consoledev.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_console.h>

static int
ofw_console_getc(void *sc, char *chp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static void
ofw_console_putc(void *sc, char ch)
{
}

static void
ofw_console_flush(void *sc)
{
}

static struct console ofw_console = {
	.c_name = "ofw",
	.c_softc = NULL,
	.c_getc = ofw_console_getc,
	.c_putc = ofw_console_putc,
	.c_flush = ofw_console_flush,
};

void
ofw_console_init(void)
{
	console_init(&ofw_console);
}
