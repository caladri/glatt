#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <io/device/device.h>
#include <io/device/driver.h>

#define	TEST_ETHER_DEV_BASE	(0x14000000)
#define	TEST_ETHER_DEV_IRQ	(3)

#define	TEST_ETHER_DEV_MTU	(0x4000)

#define	TEST_ETHER_DEV_BUFFER	(0x0000)
#define	TEST_ETHER_DEV_STATUS	(0x4000)
#define	TEST_ETHER_DEV_LENGTH	(0x4010)
#define	TEST_ETHER_DEV_COMMAND	(0x4020)

#define	TEST_ETHER_DEV_STATUS_RX_OK	(0x01)
#define	TEST_ETHER_DEV_STATUS_RX_MORE	(0x02)

#define	TEST_ETHER_DEV_COMMAND_RX	(0x00)
#define	TEST_ETHER_DEV_COMMAND_TX	(0x01)

struct tmether_softc {
	struct spinlock sc_lock;
};

static int tmether_init(struct tmether_softc *);

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
	return (0);
}

DRIVER(tmether, "testmips ethernet", NULL, DRIVER_FLAG_DEFAULT, tmether_probe, tmether_attach);
DRIVER_ATTACHMENT(tmether, "mp");
