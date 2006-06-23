#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/device/device.h>
#include <io/device/driver.h>

SET(drivers, struct driver);

static struct driver *generic;

static void driver_add_child(struct driver *, struct driver *);
static struct driver *driver_lookup_child(struct driver *, const char *);
static void driver_remove_child(struct driver *, struct driver *);

static void
driver_compile(void *arg)
{
	struct driver **driverp, *driver, *parent;

	/*
	 * First we have to find the "generic" driver which will be the root of
	 * everything else.
	 */
	for (driverp = SET_BEGIN(drivers); driverp < SET_END(drivers);
	     driverp++) {
		driver = *driverp;
		if (strcmp(driver->d_name, "generic") == 0) {
			if (generic != NULL)
				panic("%s: too many generic drivers", __func__);
			generic = driver;
			break;
		}
	}

	if (generic == NULL)
		panic("%s: could not find a generic driver", __func__);

	/*
	 * Now that "generic" is attached, add everything as a child of it.
	 */
	for (driverp = SET_BEGIN(drivers); driverp < SET_END(drivers);
	     driverp++) {
		driver = *driverp;
		if (generic == driver)
			continue;
		if (strcmp(driver->d_name, driver->d_base) == 0)
			panic("%s: driver is a child of itself.", __func__);
		driver_add_child(generic, driver);
	}

	/*
	 * Now driver_lookup works, so we can detach things from generic one
	 * by one and then find their parent and set up a hierarchy, if they
	 * have a more appropriate parent.
	 */
	for (driverp = SET_BEGIN(drivers); driverp < SET_END(drivers);
	     driverp++) {
		driver = *driverp;
		if (driver->d_base != NULL) {
			driver_remove_child(generic, driver);
			parent = driver_lookup(driver->d_base);
			if (parent == NULL)
				panic("%s: cannot find base %s for %s.",
				      __func__, driver->d_base, driver->d_name);
			driver_add_child(parent, driver);
		}
	}
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
driver_probe(struct driver *driver, struct device *device)
{
	if (driver->d_probe != NULL)
		return (driver->d_probe(device));
	else
		return (driver_probe(driver->d_parent, device));
}

static void
driver_add_child(struct driver *parent, struct driver *driver)
{
	driver->d_parent = parent;
	driver->d_peer = parent->d_children;
	parent->d_children = driver;
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

static void
driver_remove_child(struct driver *parent, struct driver *driver)
{
	struct driver *peer;

	if (parent->d_children == driver) {
		parent->d_children = driver->d_peer;
		driver->d_parent = NULL;
	} else {
		for (peer = parent->d_children; peer != NULL;
		     peer = peer->d_peer) {
			if (peer->d_peer == driver) {
				peer->d_peer = driver->d_peer;
				driver->d_parent = NULL;
				break;
			}
		}
		if (driver->d_parent != NULL)
			panic("%s: cannot remove %s from %s.", __func__,
			      driver->d_name, parent->d_name);
	}
	driver->d_peer = NULL;
}
