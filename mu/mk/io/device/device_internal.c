#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/bus_internal.h>
#include <io/device/device.h>
#include <io/device/device_internal.h>

struct device {
	int __dummy;
};

int
device_create(struct device **devicep, struct bus_instance *bi, const char *class)
{
	return (ERROR_NOT_IMPLEMENTED);
}

void
device_destroy(struct device *device)
{
}

int
device_setup(struct device *device, void *busdata)
{
	return (ERROR_NOT_IMPLEMENTED);
}
