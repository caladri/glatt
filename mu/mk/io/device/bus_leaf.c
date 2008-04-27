#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/device.h>
#include <io/device/device_internal.h>
#include <io/device/leaf.h>

struct leaf_softc {
	struct device *ls_device;
};

static void
leaf_describe(struct bus_instance *bi)
{
	struct leaf_softc *ls = bus_softc(bi);

	device_describe(ls->ls_device);
}

static int
leaf_enumerate_children(struct bus_instance *bi)
{
	return (0);
}

static int
leaf_setup(struct bus_instance *bi)
{
	struct leaf_device *ld;
	struct leaf_softc *ls;
	struct device *device;
	int error;

	ld = bus_parent_data(bi);

	error = device_create(&device, bi, ld->ld_class);
	if (error != 0)
		return (error);

	ls = bus_softc_allocate(bi, sizeof *ls);
	ls->ls_device = device;

	error = device_setup(device, ld->ld_busdata);
	if (error != 0) {
		device_destroy(device);
		return (error);
	}

	return (0);
}

BUS_INTERFACE(leafif) {
	.bus_describe = leaf_describe,
	.bus_enumerate_children = leaf_enumerate_children,
	.bus_setup = leaf_setup,
};
BUS_ATTACHMENT(leaf, NULL, leafif);
