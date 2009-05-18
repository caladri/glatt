#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <device/tmdisk.h>
#include <io/bus/bus.h>
#include <io/storage/device.h>

struct tmdisk_softc {
	struct storage_device sc_storagedev;
	unsigned sc_diskid;
	uint64_t sc_size;
};

static storage_device_read_t tmdisk_read;

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

		error = tmdisk_read(sc, NULL, offset);
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

static int
tmdisk_setup(struct bus_instance *bi)
{
	struct test_disk_busdata *bd = bus_parent_data(bi);
	struct tmdisk_softc *sc;
	int error;

	sc = bus_softc_allocate(bi, sizeof *sc);
	sc->sc_diskid = bd->d_id;
	sc->sc_size = (uint64_t)~0;
	error = tmdisk_size(sc, &sc->sc_size);
	if (error != 0)
		return (error);

	bus_set_description(bi, "testmips simulated disk #%u (%lu bytes).",
			    sc->sc_diskid, sc->sc_size);

	error = storage_device_attach(&sc->sc_storagedev,
				      TEST_DISK_DEV_BLOCKSIZE, tmdisk_read, sc);
	if (error != 0) {
		return (error);
	}

	return (0);
}

static int
tmdisk_read(void *softc, void *buf, off_t off)
{
	struct tmdisk_softc *sc = softc;
	const volatile void *src;

	if (off < 0 || off % TEST_DISK_DEV_BLOCKSIZE != 0)
		return (ERROR_INVALID);

	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_OFFSET, (uint64_t)off);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_DISKID, sc->sc_diskid);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_START, TEST_DISK_DEV_START_READ);
	switch (TEST_DISK_DEV_READ(TEST_DISK_DEV_STATUS)) {
	case TEST_DISK_DEV_STATUS_FAILURE:
		return (ERROR_UNEXPECTED);
	default:
		break;
	}

	if (buf != NULL) {
		src = TEST_DISK_DEV_FUNCTION(TEST_DISK_DEV_BLOCK);
		memcpy(buf, (const void *)(uintptr_t)src,
		       TEST_DISK_DEV_BLOCKSIZE);
	}

	return (0);
}

BUS_INTERFACE(tmdiskif) {
	.bus_setup = tmdisk_setup,
};
BUS_ATTACHMENT(tmdisk, "tmdiskc", tmdiskif);
