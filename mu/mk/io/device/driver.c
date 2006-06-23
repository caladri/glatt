#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/device/device.h>
#include <io/device/driver.h>

SET(drivers, struct driver);

static struct driver *generic;

static struct driver *driver_lookup_child(struct driver *, const char *);

static void
driver_compile(void *arg)
{
	struct driver **driverp, *driver;

	/*
	 * First we have to find the "generic" driver which will be the root of
	 * everything else.
	 */
	for (driverp = SET_BEGIN(drivers); driverp < SET_END(drivers);
	     driverp++) {
		driver = *driverp;
	}

	/*
	 * Now that "generic" is attached, add everything as a child of it.
	 */

	/*
	 * Now driver_lookup works, so we can detach things from generic one
	 * by one and then find their parent and set up a hierarchy, if they
	 * have a more appropriate parent.
	 */
}
STARTUP_ITEM(drivers, STARTUP_ROOT, STARTUP_BEFORE, driver_compile, NULL);

struct driver *
driver_lookup(const char *name)
{
	struct driver *driver;

	/*
	 * XXX
	 * Could just iterate through the set, but hierarchies are so hip.
	 */
	if (generic == NULL)
		panic("%s: no drivers installed.", __func__);
	if (name == NULL || strcmp(generic->d_name, name) == 0)
		return (generic);
	driver = driver_lookup_child(generic, name);
	if (driver == NULL)
		panic("%s: no such driver: %s", __func__, name);
	return (driver);
}

int
driver_probe(struct device *device)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static struct driver *
driver_lookup_child(struct driver *driver, const char *name)
{
	struct driver *child, *t;

	for (child = driver->d_children; child != NULL; child = child->d_peer)
		if (strcmp(child->d_name, name) == 0)
			return (child);
	for (child = driver->d_children; child != NULL; child = child->d_peer) {
		t = driver_lookup_child(child, name);
		if (t == NULL)
			continue;
		return (t);
	}
	return (NULL);
}
