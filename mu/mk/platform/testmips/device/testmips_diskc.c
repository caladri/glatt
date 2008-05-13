#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <device/tmdisk.h>
#include <io/device/bus.h>
#include <io/device/device.h>

static bool tmdiskc_disk_probe(struct bus_instance *, unsigned);

static void
tmdiskc_describe(struct bus_instance *bi)
{
	bus_printf(bi, "testmips simulated disk controller.");
}

static int
tmdiskc_enumerate_children(struct bus_instance *bi)
{
	unsigned i;

	/*
	 * Disk IDs are allocated linearly on testmips.
	 */
	for (i = 0; i <= 0xff; i++)
		if (!tmdiskc_disk_probe(bi, i))
			break;
	return (0);
}

static int
tmdiskc_setup(struct bus_instance *bi)
{
	return (0);
}

static bool
tmdiskc_disk_probe(struct bus_instance *bi, unsigned id)
{
	struct test_disk_busdata data;
	int error;

	/*
	 * If we can't read the block at offset 0, assume it isn't present.
	 */
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_OFFSET, 0);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_DISKID, id);
	TEST_DISK_DEV_WRITE(TEST_DISK_DEV_START, TEST_DISK_DEV_START_READ);
	switch (TEST_DISK_DEV_READ(TEST_DISK_DEV_STATUS)) {
	case TEST_DISK_DEV_STATUS_FAILURE:
		return (false);
	default:
		break;
	}

	/* XXX Need parent_data management like with busses?  */
	data.d_id = id;

	error = device_enumerate(bi, "tmdisk", &data);
	if (error != 0)
		panic("%s: device_enumerate failed: %m", __func__, error);
	return (true);
}

BUS_INTERFACE(tmdiskcif) {
	.bus_describe = tmdiskc_describe,
	.bus_enumerate_children = tmdiskc_enumerate_children,
	.bus_setup = tmdiskc_setup,
};
BUS_ATTACHMENT(tmdiskc, "mpbus", tmdiskcif);
