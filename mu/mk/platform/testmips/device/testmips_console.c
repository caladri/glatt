#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <cpu/memory.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>

static int
testmips_console_getc(void *sc, char *chp)
{
	volatile char *getcp = sc;
	char ch;

	ch = *getcp;
	if (ch == '\0')
		return (ERROR_AGAIN);
	switch (ch) {
	case '\r':
		*chp = '\n';
		break;
	default:
		*chp = ch;
		break;
	}
	return (0);
}

static void
testmips_console_putc(void *sc, char ch)
{
	volatile char *putcp = sc;
	*putcp = ch;
}

static void
testmips_console_flush(void *sc)
{
	(void)sc;
}

static struct console testmips_console = {
	.c_name = "testmips",
	.c_softc = XKPHYS_MAP(XKPHYS_UC, 0x10000000),
	.c_getc = testmips_console_getc,
	.c_putc = testmips_console_putc,
	.c_flush = testmips_console_flush,
};

void
platform_console_init(void)
{
	console_init(&testmips_console);
}
