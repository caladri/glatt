#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

static struct pool device_pool;

/*
 * XXX
 * Should be called with the parent locked.
 */
int
device_create(struct device **devicep, struct device *parent,
	      struct driver *driver)
{
	struct device *device;
	int error;

	device = pool_allocate(&device_pool);
	if (device == NULL)
		return (ERROR_EXHAUSTED);
	if (parent == NULL)
		parent = device_root();
	error = device_init(device, parent, driver);
	if (error != 0) {
		pool_free(&device_pool, device);
		return (error);
	}
	if (devicep != NULL)
		*devicep = device;
	return (0);
}

/*
 * XXX
 * Should be called with the parent locked.
 */
int
device_init(struct device *device, struct device *parent, struct driver *driver)
{
	int error;

	ASSERT(device != NULL, "need a device allocated to initialize");
	ASSERT(driver != NULL, "need a driver for a device");

	spinlock_init(&device->d_lock, driver->d_name);
	device->d_unit = driver->d_nextunit++;
	if (parent != NULL) {
		DEVICE_LOCK(parent);
		DEVICE_LOCK(device);
		device->d_parent = parent;
		STAILQ_INSERT_TAIL(&parent->d_children, device, d_link);
	} else {
		DEVICE_LOCK(device);
		device->d_parent = NULL;
	}
	STAILQ_INIT(&device->d_children);
	device->d_driver = driver;
	device->d_state = DEVICE_PROBING;
	device->d_desc = driver->d_desc;

	error = driver_probe(device->d_driver, device);
	if (error != 0) {
		if (parent != NULL) {
			device->d_parent = NULL;
			STAILQ_REMOVE(&parent->d_children, device,
				      struct device, d_link);
			DEVICE_UNLOCK(device);
			DEVICE_UNLOCK(parent);
		} else {
			DEVICE_UNLOCK(device);
		}
		device->d_state = DEVICE_DYING;
		return (error);
	}
	device->d_state = DEVICE_ATTACHED;

	/*
	 * XXX probe for possible children.
	 */

	DEVICE_UNLOCK(device);
	if (parent != NULL)
		DEVICE_UNLOCK(parent);
	return (0);
}

void
device_printf(struct device *device, const char *fmt, ...)
{
	va_list ap;

	kcprintf("%s%u: ", device->d_driver->d_name, device->d_unit);
	va_start(ap, fmt);
	kcvprintf(fmt, ap);
	va_end(ap);
	kcprintf("\n");
}

static void
device_pool_setup(void *arg)
{
	int error;

	error = pool_create(&device_pool, "DEVICE", sizeof (struct device),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(device_pool, STARTUP_POOL, STARTUP_FIRST, device_pool_setup, NULL);
