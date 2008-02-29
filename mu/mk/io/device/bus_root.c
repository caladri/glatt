#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/bus.h>

static int
root_enumerate_children(struct bus_instance *bi)
{
	return (0);
}

static int
root_resource_manage(struct bus_instance *bi, struct bus_resource *req, size_t nreq)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static int
root_setup(struct bus_instance *bi, void *busdata)
{
	return (0);
}

BUS_INTERFACE(rootif) {
	.bus_enumerate_children = root_enumerate_children,
	.bus_resource_manage = root_resource_manage,
	.bus_setup = root_setup,
};
BUS_ATTACHMENT(root, NULL, rootif);
