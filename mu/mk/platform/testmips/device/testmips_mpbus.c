#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/pcpu.h>
#include <io/device/bus.h>

static int
mpbus_enumerate_children(struct bus_instance *bi)
{
	return (bus_enumerate_children(bi));
}

static int
mpbus_setup(struct bus_instance *bi, void *busdata)
{
	return (0);
}

BUS_INTERFACE(mpbusif) {
	.bus_enumerate_children = mpbus_enumerate_children,
	.bus_setup = mpbus_setup,
};
BUS_ATTACHMENT(mpbus, "cpu", mpbusif);
