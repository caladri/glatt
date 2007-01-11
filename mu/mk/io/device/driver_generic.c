#include <core/types.h>
#include <core/error.h>
#include <io/device/driver.h>

static int
generic_probe(struct device *device)
{
	return (0);
}

static int
generic_attach(struct device *device)
{
	return (0);
}

DRIVER(generic, "Generic", NULL, DRIVER_FLAG_DEFAULT, generic_probe, generic_attach);
