#include <core/types.h>
#include <core/error.h>
#include <io/console/consoledev.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_console.h>
#include <io/ofw/ofw_functions.h>

struct ofw_console_softc {
	ofw_instance_t sc_instance;
};

static struct ofw_console_softc ofw_console_softc;

static int
ofw_console_getc(void *scp, char *chp)
{
	struct ofw_console_softc *sc = scp;

	(void)sc;

	return (ERROR_NOT_IMPLEMENTED);
}

static void
ofw_console_putc(void *scp, char ch)
{
	struct ofw_console_softc *sc = scp;
	size_t len;
	int error;

	error = ofw_write(sc->sc_instance, &ch, 1, &len);
	if (error != 0)
		panic("%s: ofw_wrie failed: %m", __func__, error);

	if (len != 1)
		panic("%s: ofw_write returned len %zu", __func__, len);
}

static void
ofw_console_flush(void *scp)
{
	struct ofw_console_softc *sc = scp;

	(void)sc;
}

static struct console ofw_console = {
	.c_name = "ofw",
	.c_softc = &ofw_console_softc,
	.c_getc = ofw_console_getc,
	.c_putc = ofw_console_putc,
	.c_flush = ofw_console_flush,
};

void
ofw_console_init(void)
{
	struct ofw_console_softc *sc = &ofw_console_softc;
	ofw_package_t chosen;
	size_t len;
	int error;

	error = ofw_finddevice("/chosen", &chosen);
	if (error != 0)
		panic("%s: ofw_finddevice failed: %m", __func__, error);

	error = ofw_getprop(chosen, "stdout", &sc->sc_instance,
			    sizeof sc->sc_instance, &len);
	if (error != 0)
		panic("%s: ofw_getprop failed: %m", __func__, error);

	if (len != sizeof sc->sc_instance)
		panic("%s: ofw_getprop returned len %zu", __func__, len);

	console_init(&ofw_console);
}
