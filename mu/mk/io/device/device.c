#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/device_internal.h>
#include <io/device/leaf.h>

int
device_enumerate(struct bus_instance *bi, const char *class, void *busdata)
{
	struct leaf_device ld;
	int error;

	ld.ld_class = class;
	ld.ld_busdata = busdata;

	error = bus_enumerate_child(bi, "leaf", &ld);
	if (error != 0)
		return (error);

	return (0);
}

void
device_printf(struct device *device, const char *fmt, ...)
{
	va_list ap;

	/* XXX Unit number?  */
	kcprintf("%s@", device_name(device));
	va_start(ap, fmt);
	bus_vprintf(device_parent(device), fmt, ap);
	va_end(ap);
}
