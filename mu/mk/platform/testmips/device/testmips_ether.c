#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <io/device/device.h>
#include <io/device/driver.h>

#include <io/device/console/console.h>

#define	TEST_ETHER_DEV_BASE	(0x14000000)
#define	TEST_ETHER_DEV_IRQ	(3)

#define	TEST_ETHER_DEV_MTU	(0x4000)

#define	TEST_ETHER_DEV_BUFFER	(0x0000)
#define	TEST_ETHER_DEV_STATUS	(0x4000)
#define	TEST_ETHER_DEV_LENGTH	(0x4010)
#define	TEST_ETHER_DEV_COMMAND	(0x4020)

#define	TEST_ETHER_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_ETHER_DEV_BASE + (f))
#define	TEST_ETHER_DEV_READ(f)						\
	(volatile uint64_t)*TEST_ETHER_DEV_FUNCTION(f)
#define	TEST_ETHER_DEV_WRITE(f, v)					\
	*TEST_ETHER_DEV_FUNCTION(f) = (v)

#define	TEST_ETHER_DEV_STATUS_RX_OK	(0x01)
#define	TEST_ETHER_DEV_STATUS_RX_MORE	(0x02)

#define	TEST_ETHER_DEV_COMMAND_RX	(0x00)
#define	TEST_ETHER_DEV_COMMAND_TX	(0x01)

struct tmether_softc {
	struct spinlock sc_lock;
};

#define	TMETHER_LOCK(sc)	spinlock_lock(&(sc)->sc_lock)
#define	TMETHER_UNLOCK(sc)	spinlock_unlock(&(sc)->sc_lock)

static int tmether_init(struct tmether_softc *);
static void tmether_interrupt(void *, int);

static int
tmether_probe(struct device *device)
{
	if (device->d_unit != 0)
		return (ERROR_NOT_FOUND);
	return (0);
}

static int
tmether_attach(struct device *device)
{
	struct tmether_softc *sc;
	int error;

	sc = malloc(sizeof *sc);

	device->d_softc = sc;

	error = tmether_init(sc);
	if (error != 0) {
		free(sc);
		return (error);
	}

	return (0);
}

static int
tmether_init(struct tmether_softc *sc)
{
	spinlock_init(&sc->sc_lock, "testmips ethernet");
	TMETHER_LOCK(sc);
	cpu_interrupt_establish(TEST_ETHER_DEV_IRQ, tmether_interrupt, sc);
	TMETHER_UNLOCK(sc);
	return (0);
}

static void
tmether_interrupt(void *arg, int interrupt)
{
	struct tmether_softc *sc = arg;

	TMETHER_LOCK(sc);
	switch (TEST_ETHER_DEV_READ(TEST_ETHER_DEV_STATUS)) {
	case TEST_ETHER_DEV_STATUS_RX_OK:
		kcprintf("TEST_ETHER_DEV_STATUS_RX_OK\n");
		break;
	case TEST_ETHER_DEV_STATUS_RX_MORE:
		kcprintf("TEST_ETHER_DEV_STATUS_RX_MORE\n");
		break;
	default:
		panic("%s: unexpected status.", __func__);
		break;
	}
	TMETHER_UNLOCK(sc);
}

DRIVER(tmether, "testmips ethernet", NULL, DRIVER_FLAG_DEFAULT, tmether_probe, tmether_attach);
DRIVER_ATTACHMENT(tmether, "mpbus");
