#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

SET(drivers, struct driver);
SET(driver_attachments, struct driver_attachment);

static struct driver *generic;

static void driver_add_attachment(struct driver *, struct driver_attachment *);
static void driver_add_child(struct driver *, struct driver *);
static int driver_attach(struct driver *, struct device *);
static struct driver *driver_lookup_child(struct driver *, const char *);
static void driver_remove_child(struct driver *, struct driver *);

static void
driver_compile(void *arg)
{
	struct driver **driverp, *driver, *parent;
	struct driver_attachment **attachmentp, *attachment;

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
		if (driver->d_base != NULL &&
		    strcmp(driver->d_name, driver->d_base) == 0)
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
			driver_add_child(parent, driver);
		}
	}

	/*
	 * Now sort out all of our driver attachments.
	 */
	for (attachmentp = SET_BEGIN(driver_attachments);
	     attachmentp < SET_END(driver_attachments); attachmentp++) {
		attachment = *attachmentp;
		parent = driver_lookup(attachment->da_parent);
		driver_add_attachment(parent, attachment);
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
	struct driver_attachment *attachment;
	int error;

	if (driver->d_probe != NULL) {
		error = driver->d_probe(device);
		if (error == 0 && device->d_parent != NULL) {
			device_printf(device, "<%s> on %s%u", device->d_desc,
				      device->d_parent->d_driver->d_name,
				      device->d_parent->d_unit);
			error = driver_attach(device->d_driver, device);
		}
	} else
		error = driver_probe(driver->d_parent, device);
	if (error != 0)
		return (error);
	STAILQ_FOREACH(attachment, &driver->d_attachments, da_link) {
		/*
		 * XXX
		 * How do we figure out how many children to try and create for
		 * each device with an attachment?
		 */
		error = device_create(NULL, device, attachment->da_driver);
	}
	return (0);
}

static void
driver_add_attachment(struct driver *parent, struct driver_attachment *attachment)
{
	STAILQ_INSERT_TAIL(&parent->d_attachments, attachment, da_link);
}

static void
driver_add_child(struct driver *parent, struct driver *driver)
{
	driver->d_parent = parent;
	STAILQ_INSERT_TAIL(&parent->d_children, driver, d_link);
}

static int
driver_attach(struct driver *driver, struct device *device)
{
	int error;

	if (driver->d_attach != NULL) {
		error = driver->d_attach(device);
		if (error != 0) {
			device_printf(device, "%s attach failed: %m",
				      driver->d_name, error);
		}
		return (error);
	}
	error = driver_attach(driver->d_parent, device);
	return (error);
}

static struct driver *
driver_lookup_child(struct driver *driver, const char *name)
{
	struct driver *child, *t;

	STAILQ_FOREACH(child, &driver->d_children, d_link)
		if (strcmp(child->d_name, name) == 0)
			return (child);
	STAILQ_FOREACH(child, &driver->d_children, d_link) {
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
	STAILQ_REMOVE(&parent->d_children, driver, struct driver, d_link);
	driver->d_parent = NULL;
}
