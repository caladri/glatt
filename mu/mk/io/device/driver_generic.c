#include <core/types.h>
#include <core/error.h>
#include <io/device/driver.h>

static int
generic_probe(struct device *device)
{
	return (0);
}

DRIVER(generic, NULL, generic_probe);
