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
	uint64_t sc_size;
};

static int
tmdisk_read(struct tmdisk_softc *sc, uint64_t offset)
{
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_OFFSET, offset);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_DISKID, sc->sc_diskid);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_START, TEST_DISK_DEV_START_READ);
	switch (TEST_DISK_DEV_READ(TEST_DISK_DEV_STATUS)) {
	case TEST_DISK_DEV_STATUS_FAILURE:
		return (ERROR_UNEXPECTED);
	default:
		break;
	}
	return (0);
}

static int
tmdisk_size(struct tmdisk_softc *sc, uint64_t *sizep)
{
	uint64_t offset, ogood;
	uint64_t m, s;
	int error;

	m = 1;
	s = 3;
	ogood = 0;

	for (;;) {
		offset = (ogood * s) + (m * TEST_DISK_DEV_BLOCKSIZE);

		error = tmdisk_read(sc, offset);
		if (error != 0) {
			if (m == 1 && s == 1) {
				*sizep = ogood + TEST_DISK_DEV_BLOCKSIZE;
				return (0);
			}
			if (m > 1)
				m /= 2;
			if (s > 1)
				s--;
			continue;
		}
		if (ogood == offset) {
			m = 1;
			continue;
		}
		ogood = offset;
		m++;
	}
	
	return (ERROR_UNEXPECTED);
}

static void
tmdisk_describe(struct device *device)
{
	struct tmdisk_softc *sc = device_softc(device);

	device_printf(device, "testmips simulated disk #%u (%lu bytes).", sc->sc_diskid, sc->sc_size);
}

static int
tmdisk_setup(struct device *device, void *busdata)
{
	struct test_disk_busdata *bd = busdata;
	struct tmdisk_softc *sc;
	int error;

	sc = device_softc_allocate(device, sizeof *sc);
	sc->sc_diskid = bd->d_id;
	sc->sc_size = (uint64_t)~0;
	error = tmdisk_size(sc, &sc->sc_size);
	if (error != 0)
		return (error);

	return (0);
}

DEVICE_INTERFACE(tmdiskif) {
	.device_describe = tmdisk_describe,
	.device_setup = tmdisk_setup,
};
DEVICE_ATTACHMENT(tmdisk, "tmdiskc", tmdiskif);
