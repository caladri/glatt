#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <device/tmdisk.h>
#include <io/device/device.h>

struct tmdisk_softc {
	unsigned sc_diskid;
};

static void
tmdisk_describe(struct device *device)
{
	struct tmdisk_softc *sc = device_softc(device);

	device_printf(device, "testmips simulated disk #%u.", sc->sc_diskid);
}

static int
tmdisk_setup(struct device *device, void *busdata)
{
	struct test_disk_busdata *bd = busdata;
	struct tmdisk_softc *sc;

	sc = device_softc_allocate(device, sizeof *sc);
	sc->sc_diskid = bd->d_id;

	return (0);
}

DEVICE_INTERFACE(tmdiskif) {
	.device_describe = tmdisk_describe,
	.device_setup = tmdisk_setup,
};
DEVICE_ATTACHMENT(tmdisk, "tmdiskc", tmdiskif);
