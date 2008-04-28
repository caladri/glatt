#include <core/types.h>
#include <core/error.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/pcpu.h>
#include <io/device/bus.h>
#include <io/device/device.h>

static const char *mpbus_children[] = {
	"tmether",
	"tmfb",
	NULL,
};

static int
mpbus_enumerate_children(struct bus_instance *bi)
{
	const char **childp;
	int error;

	for (childp = mpbus_children; *childp != NULL; childp++) {
		error = device_enumerate(bi, *childp, NULL);
		if (error != 0)
			bus_printf(bi, "%s not present or enabled: %m", *childp,
				   error);
	}

	return (bus_enumerate_children(bi));
}

static int
mpbus_setup(struct bus_instance *bi)
{
	if ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0) {
		panic("%s: shouldn't be enumerating on application processor.",
		      __func__);
	}
	return (0);
}

BUS_INTERFACE(mpbusif) {
	.bus_enumerate_children = mpbus_enumerate_children,
	.bus_setup = mpbus_setup,
};
BUS_ATTACHMENT(mpbus, "mp", mpbusif);
