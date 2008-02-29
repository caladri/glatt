#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/bus_internal.h>
#include <io/device/console/console.h>

/*
 * External interface to bus management.
 */

int
bus_enumerate_child(struct bus_instance *bi, const char *class, void *busdata)
{
	struct bus_instance *child;
	struct bus *bus;
	int error;

	error = bus_lookup(&bus, class);
	if (error != 0) {
		/*
		 * XXX
		 * Maybe it's a leaf device?  Or do we want any leaf device
		 * to be off a special leaf bus?
		 */
		return (error);
	}

	error = bus_instance_create(&child, bi, bus);
	if (error != 0) {
		return (error);
	}

	error = bus_instance_setup(child, busdata);
	if (error != 0) {
		bus_instance_destroy(child);
		return (error);
	}

	return (0);
}
