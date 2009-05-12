#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/device/bus.h>
#include <io/device/bus_internal.h>

/*
 * External interface to bus management.
 */

int
bus_enumerate_child(struct bus_instance *bi, const char *class,
		    struct bus_instance **childp)
{
	struct bus_instance *child;
	struct bus *bus;
	int error;

	ASSERT(childp != NULL, "Parent bus must be able to set up the child.");

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
	if (error != 0)
		return (error);

	*childp = child;

	return (0);
}

int
bus_enumerate_child_generic(struct bus_instance *bi, const char *class)
{
	struct bus_instance *child;
	int error;

	error = bus_enumerate_child(bi, class, &child);
	if (error != 0)
		return (error);

	error = bus_setup_child(child);
	if (error != 0)
		return (error);

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

void *
bus_parent_data(struct bus_instance *bi)
{
	void *pdata;

	pdata = bus_instance_parent_data(bi);
	return (pdata);
}

void *
bus_parent_data_allocate(struct bus_instance *bi, size_t size)
{
	void *pdata;

	pdata = bus_instance_parent_data_allocate(bi, size);
	return (pdata);
}

void
bus_printf(struct bus_instance *bi, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bus_vprintf(bi, fmt, ap);
	va_end(ap);
}

void
bus_set_description(struct bus_instance *bi, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bus_instance_set_description(bi, fmt, ap);
	va_end(ap);
}

int
bus_setup_child(struct bus_instance *bi)
{
	int error;

	error = bus_instance_setup(bi);
	if (error != 0) {
		/* XXX Should we leave this up to the parent?  */
		bus_instance_destroy(bi);
		return (error);
	}

	return (0);
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

void
bus_vprintf(struct bus_instance *bi, const char *fmt, va_list ap)
{
	bus_instance_vprintf(bi, fmt, ap);
}
