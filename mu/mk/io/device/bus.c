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

	if (strcmp("root", class) == 0)
		return (ERROR_NOT_PERMITTED);

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

int
bus_enumerate_children(struct bus_instance *bi)
{
	int error;

	error = bus_instance_enumerate_children(bi);
	if (error != 0)
		return (error);
	return (0);
}

const char *
bus_name(struct bus_instance *bi)
{
	return (bus_instance_name(bi));
}

struct bus_instance *
bus_parent(struct bus_instance *bi)
{
	return (bus_instance_parent(bi));
}

void
bus_printf(struct bus_instance *bi, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bus_instance_vprintf(bi, fmt, ap);
	va_end(ap);
}

void *
bus_softc(struct bus_instance *bi)
{
	void *sc;

	sc = bus_instance_softc(bi);
	return (sc);
}

void *
bus_softc_allocate(struct bus_instance *bi, size_t size)
{
	void *sc;

	sc = bus_instance_softc_allocate(bi, size);
	return (sc);
}
