#include <core/types.h>
#include <core/startup.h>
#include <db/db.h>
#include <db/db_command.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

static struct device device_root_storage;

DRIVER(root, NULL, NULL);

static void
device_startup_root(void *arg)
{
	struct driver *driver;
	int error;

	driver = driver_lookup("root");
	if (driver == NULL)
		panic("%s: driver_lookup failed.", __func__);
	error = device_init(&device_root_storage, NULL, driver);
	if (error != 0)
		panic("%s: device_init failed: %m", __func__, error);
}
STARTUP_ITEM(device, STARTUP_ROOT, STARTUP_FIRST, device_startup_root, NULL);

struct device *
device_root(void)
{
	struct device *root;

	root = &device_root_storage;
	if (root->d_state != DEVICE_ATTACHED)
		panic("%s: root not attached.", __func__);
	return (root);
}

static void
device_db_dump_path(struct device *device)
{
	if (device == NULL) {
		kcprintf("device:/");
		return;
	}
	device_db_dump_path(device->d_parent);
	kcprintf("/%s%u", device->d_driver->d_name, device->d_unit);
}

static void
device_db_dump_tree(struct device *device)
{
	struct device *child;

	if (device == NULL)
		return;
	DEVICE_LOCK(device);
	device_db_dump_path(device);
	kcprintf("\n\t^ State: %u\n", device->d_state);
	for (child = device->d_children; child != NULL; child = child->d_peer) {
		device_db_dump_tree(child);
	}
	DEVICE_UNLOCK(device);
}

static void
device_db_dump(void)
{
	kcprintf("Dumping device tree.\n");
	device_db_dump_tree(&device_root_storage);
	kcprintf("Finished.\n");
}
DB_COMMAND(device_dump, device_db_dump, "Dump a listing of devices.");
