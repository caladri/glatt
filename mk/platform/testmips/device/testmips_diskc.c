#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <device/tmdisk.h>
#include <io/device/bus.h>

static bool tmdiskc_disk_probe(struct bus_instance *, unsigned);

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
	bus_set_description(bi, "testmips simulated disk controller.");
	return (0);
}

static bool
tmdiskc_disk_probe(struct bus_instance *bi, unsigned id)
{
	struct test_disk_busdata *bd;
	struct bus_instance *child;
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

	error = bus_enumerate_child(bi, "tmdisk", &child);
	if (error != 0)
		panic("%s: bus_enumerate_child failed: %m", __func__, error);

	bd = bus_parent_data_allocate(child, sizeof *bd);
	bd->d_id = id;

	error = bus_setup_child(child);
	if (error != 0)
		panic("%s: bus_setup_child failed: %m", __func__, error);
	return (true);
}

BUS_INTERFACE(tmdiskcif) {
	.bus_enumerate_children = tmdiskc_enumerate_children,
	.bus_setup = tmdiskc_setup,
};
BUS_ATTACHMENT(tmdiskc, "mpbus", tmdiskcif);
