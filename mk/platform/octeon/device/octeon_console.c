#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <device/mio.h>
#include <io/console/console.h>
#include <io/console/consoledev.h>

static int
octeon_console_getc(void *sc, char *chp)
{
	uint8_t ch;
	int error;

	(void)sc;

	octeon_mio_bus_barrier();
	error = octeon_mio_uart_read(0, &ch);
	if (error != 0)
		return (error);
	octeon_mio_bus_barrier();

	switch ((char)ch) {
	case '\r':
		*chp = '\n';
		break;
	default:
		*chp = (char)ch;
		break;
	}
	return (0);
}


static void
octeon_console_putc(void *sc, char ch)
{
	(void)sc;

	if (ch == '\n')
		octeon_mio_uart_write(0, (uint8_t)'\r');

	octeon_mio_uart_write(0, (uint8_t)ch);
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

	octeon_mio_bus_barrier();
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
