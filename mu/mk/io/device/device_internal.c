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
	struct bus_instance *d_instance;
	struct device_attachment *d_attachment;
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
	if (devicep != NULL)
		*devicep = device;
	return (0);
}

void
device_destroy(struct device *device)
{
	pool_free(device);
}

struct bus_instance *
device_parent(struct device *device)
{
	return (device->d_instance);
}

int
device_setup(struct device *device, void *busdata)
{
	int error;

	error = device->d_attachment->da_interface->device_setup(device, busdata);
	if (error != 0)
		return (error);
	return (0);
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
