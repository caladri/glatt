#include <core/types.h>
#include <core/error.h>
#include <io/device/bus.h>

static void
root_describe(struct bus_instance *bi)
{
}

static int
root_enumerate_children(struct bus_instance *bi)
{
	int error;

	error = bus_enumerate_children(bi);
	if (error != 0)
		return (error);
	return (0);
}

static int
root_setup(struct bus_instance *bi)
{
	return (0);
}

BUS_INTERFACE(rootif) {
	.bus_describe = root_describe,
	.bus_enumerate_children = root_enumerate_children,
	.bus_setup = root_setup,
};
BUS_ATTACHMENT(root, NULL, rootif);
