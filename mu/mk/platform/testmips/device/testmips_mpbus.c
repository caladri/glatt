#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/pcpu.h>
#include <io/device/device.h>
#include <io/device/driver.h>

static int
mpbus_probe(struct device *device)
{
	if ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0)
		return (ERROR_NOT_FOUND);
	return (0);
}

static int
mpbus_attach(struct device *device)
{
	return (0);
}

DRIVER(mpbus, "mp bus attachment", NULL, DRIVER_FLAG_DEFAULT, mpbus_probe, mpbus_attach);
DRIVER_ATTACHMENT(mpbus, "cpu");
