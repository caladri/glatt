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
	(void)bi;
	(void)req;
	(void)nreq;

	return (ERROR_NOT_IMPLEMENTED);
}

BUS_INTERFACE(rootif) {
	.bus_enumerate_children = root_enumerate_children,
	.bus_resource_manage = root_resource_manage,
};
BUS_ATTACHMENT(root, NULL, rootif);
