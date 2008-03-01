#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/device.h>
#include <io/device/device_internal.h>
#include <io/device/leaf.h>

static int
leaf_enumerate_children(struct bus_instance *bi)
{
	return (0);
}

static int
leaf_resource_manage(struct bus_instance *bi, struct bus_resource *req, size_t nreq)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static int
leaf_setup(struct bus_instance *bi, void *busdata)
{
	struct leaf_device *ld = busdata;
	struct device *device;
	int error;

	error = device_create(&device, bi, ld->ld_class);
	if (error != 0)
		return (error);

	error = device_setup(device, ld->ld_busdata);
	if (error != 0) {
		device_destroy(device);
		return (error);
	}

	return (0);
}

BUS_INTERFACE(leafif) {
	.bus_enumerate_children = leaf_enumerate_children,
	.bus_resource_manage = leaf_resource_manage,
	.bus_setup = leaf_setup,
};
BUS_ATTACHMENT(leaf, NULL, leafif);
