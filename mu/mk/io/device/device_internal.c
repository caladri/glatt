#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/bus_internal.h>
#include <io/console/console.h>
#include <io/device/device.h>
#include <io/device/device_internal.h>
#include <io/device/leaf.h>

struct device {
	struct bus_instance *d_instance;
	struct device_attachment *d_attachment;
	void *d_softc;
};
static struct pool device_pool;

SET(device_attachments, struct device_attachment);

static int device_attachment_find(struct device_attachment **, struct bus_instance *, const char *);

int
device_create(struct device **devicep, struct bus_instance *bi, const char *class)
{
	struct device_attachment *attachment;
	struct device *device;
	int error;

	error = device_attachment_find(&attachment, bi, class);
	if (error != 0)
		return (error);
	device = pool_allocate(&device_pool);
	device->d_instance = bi;
	device->d_attachment = attachment;
	device->d_softc = NULL;
	if (devicep != NULL)
		*devicep = device;
	return (0);
}

void
device_describe(struct device *device)
{
	if (device->d_attachment->da_interface->device_describe == NULL) {
		device_printf(device, "<%m>", ERROR_NOT_IMPLEMENTED);
		return;
	}
	device->d_attachment->da_interface->device_describe(device);
}

void
device_destroy(struct device *device)
{
	if (device->d_softc != NULL)
		free(device->d_softc);
	pool_free(device);
}

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

struct bus_instance *
device_parent(struct device *device)
{
	return (device->d_instance);
}

void
device_printf(struct device *device, const char *fmt, ...)
{
	va_list ap;

	/* XXX Unit number?  */
	kcprintf("%s@", device->d_attachment->da_name);
	va_start(ap, fmt);
	bus_vprintf(device_parent(device), fmt, ap);
	va_end(ap);
}

int
device_setup(struct device *device, void *busdata)
{
	int error;

	error = device->d_attachment->da_interface->device_setup(device, busdata);
	if (error != 0) {
#ifdef VERBOSE
		device_printf(device, "device_setup: %m", error);
#endif
		return (error);
	}
	return (0);
}

void *
device_softc(struct device *device)
{
	ASSERT(device->d_softc != NULL, "Don't ask for what isn't there.");
	return (device->d_softc);
}

void *
device_softc_allocate(struct device *device, size_t size)
{
	ASSERT(device->d_softc == NULL, "Can't create two softcs.");
	device->d_softc = malloc(size);
	return (device->d_softc);
}

static int
device_attachment_find(struct device_attachment **attachment2p,
		       struct bus_instance *bi, const char *class)
{
	struct device_attachment **attachmentp;
	struct bus_instance *parent;
	const char *busname;

	parent = bus_parent(bi);
	busname = bus_name(parent);
	
	for (attachmentp = SET_BEGIN(device_attachments);
	     attachmentp < SET_END(device_attachments); attachmentp++) {
		struct device_attachment *attachment = *attachmentp;

		if (strcmp(attachment->da_name, class) != 0)
			continue;
		if (strcmp(attachment->da_parent, busname) != 0)
			continue;
		*attachment2p = attachment;
		return (0);
	}
	return (ERROR_NOT_FOUND);
}

static void
device_startup(void *arg)
{
	int error;

	error = pool_create(&device_pool, "device", sizeof (struct device),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(device, STARTUP_DRIVERS, STARTUP_BEFORE, device_startup, NULL);
