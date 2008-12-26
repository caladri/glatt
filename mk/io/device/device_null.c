#include <core/types.h>
#include <core/error.h>
#include <io/device/device.h>

static int
null_setup(struct device *device, void *busdata)
{
	return (ERROR_INVALID);
}

DEVICE_INTERFACE(nullif) {
	.device_setup = null_setup,
};
DEVICE_ATTACHMENT(null, NULL, nullif);
