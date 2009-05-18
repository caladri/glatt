#include <core/types.h>
#include <io/bus/bus.h>

static int
mainbus_enumerate_children(struct bus_instance *bi)
{
	int error;

	error = bus_enumerate_child_generic(bi, "mp");
	if (error != 0)
		return (error);

	return (0);
}

static int
mainbus_setup(struct bus_instance *bi)
{
	return (0);
}

BUS_INTERFACE(mainbusif) {
	.bus_enumerate_children = mainbus_enumerate_children,
	.bus_setup = mainbus_setup,
};
BUS_ATTACHMENT(mainbus, "root", mainbusif);
