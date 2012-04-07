#include <core/types.h>
#include <core/error.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <io/bus/bus.h>
#ifdef VERBOSE
#include <io/console/console.h>
#endif
#include <io/console/consoledev.h>

#define	TEST_CONSOLE_DEV_BASE	(0x10000000)
#define	TEST_CONSOLE_DEV_IRQ	(2)

#define	TEST_CONSOLE_DEV_MAP()						\
	(volatile uint64_t *)XKPHYS_MAP(CCA_UC, TEST_CONSOLE_DEV_BASE)
#define	TEST_CONSOLE_DEV_READ()						\
	(volatile uint64_t)*TEST_CONSOLE_DEV_MAP()
#define	TEST_CONSOLE_DEV_WRITE(v)					\
	*TEST_CONSOLE_DEV_MAP() = (v)

#define	TMCONS_LOCK(sc)	spinlock_lock(&(sc)->sc_lock)
#define	TMCONS_UNLOCK(sc)	spinlock_unlock(&(sc)->sc_lock)

static int testmips_console_early_getc(void *, char *);
static void testmips_console_putc(void *, char);
static void testmips_console_puts(void *, const char *, size_t);
static void tmcons_interrupt(void *, int);

struct tmcons_softc {
	struct spinlock sc_lock;
	char sc_buf[1024];
	unsigned sc_buffo;
	unsigned sc_avail;
	struct console sc_console;
};

static int
testmips_console_early_getc(void *arg, char *chp)
{
	char ch;

	(void)arg;

	ch = TEST_CONSOLE_DEV_READ();
	if (ch == '\0')
		return (ERROR_AGAIN);
	switch (ch) {
	case '\r':
		*chp = '\n';
		break;
	case '\177':
		*chp = '\010';
		break;
	default:
		*chp = ch;
		break;
	}
	return (0);
}

static int
testmips_console_getc(void *arg, char *chp)
{
	struct tmcons_softc *sc = arg;
	int error;

	TMCONS_LOCK(sc);
	if (sc->sc_avail == 0) {
		error = testmips_console_early_getc(NULL, chp);
		if (error != 0) {
			TMCONS_UNLOCK(sc);
			return (error);
		}
		TMCONS_UNLOCK(sc);
		return (0);
	}
	*chp = sc->sc_buf[sc->sc_buffo];
	sc->sc_buffo = (sc->sc_buffo + 1) % sizeof sc->sc_buffo;
	sc->sc_avail--;
	TMCONS_UNLOCK(sc);

	return (0);
}

static void
testmips_console_putc(void *arg, char ch)
{
	(void)arg;

	TEST_CONSOLE_DEV_WRITE(ch);
}

static void
testmips_console_puts(void *arg, const char *s, size_t len)
{
	(void)arg;

	while (len--)
		TEST_CONSOLE_DEV_WRITE(*s++);
}

static struct console testmips_early_console = {
	.c_name = "testmips early",
	.c_softc = NULL,
	.c_getc = testmips_console_early_getc,
	.c_putc = testmips_console_putc,
	.c_puts = testmips_console_puts,
};

void
platform_console_init(void)
{
	console_init(&testmips_early_console);
}

static int
tmcons_setup(struct bus_instance *bi)
{
	struct tmcons_softc *sc;

	bus_set_description(bi, "testmips simulated console device.");

	sc = bus_softc_allocate(bi, sizeof *sc);

	spinlock_init(&sc->sc_lock, "testmips console", SPINLOCK_FLAG_DEFAULT);
	sc->sc_buffo = 0;
	sc->sc_avail = 0;
	sc->sc_console = testmips_early_console; /* Copy putc fields.  */
	sc->sc_console.c_softc = sc;
	sc->sc_console.c_name = "testmips";
	sc->sc_console.c_getc = testmips_console_getc;

	console_init(&sc->sc_console);

	cpu_interrupt_establish(TEST_CONSOLE_DEV_IRQ, tmcons_interrupt, sc);
	return (0);
}

static void
tmcons_interrupt(void *arg, int interrupt)
{
	struct tmcons_softc *sc = arg;
	bool need_wakeup;
	int error;

	TMCONS_LOCK(sc);
	need_wakeup = sc->sc_avail == 0;
	for (;;) {
		if (sc->sc_avail == sizeof sc->sc_buf) {
			while (TEST_CONSOLE_DEV_READ() != 0)
				continue;
			TMCONS_UNLOCK(sc);
#ifdef VERBOSE
			kcprintf("%s: buffer full, dropped characters.\n", __func__);
#endif
			return;
		}
		error = testmips_console_early_getc(NULL, &sc->sc_buf[(sc->sc_buffo + sc->sc_avail) % sizeof sc->sc_buf]);
		if (error != 0)
			break;
		sc->sc_avail++;
	}
	if (need_wakeup) {
		/*
		 * XXX
		 * Indicate input available on console.
		 */
	}
	TMCONS_UNLOCK(sc);
}

BUS_INTERFACE(tmconsif) {
	.bus_setup = tmcons_setup,
};
BUS_ATTACHMENT(tmcons, "mpbus", tmconsif);
