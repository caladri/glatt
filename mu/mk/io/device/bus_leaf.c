#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/bus.h>

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
	return (0);
}

BUS_INTERFACE(leafif) {
	.bus_enumerate_children = leaf_enumerate_children,
	.bus_resource_manage = leaf_resource_manage,
	.bus_setup = leaf_setup,
};
BUS_ATTACHMENT(leaf, NULL, leafif);
