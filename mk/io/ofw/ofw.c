#include <core/types.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_console.h>

static void (*ofw_entry)(void *);

void
ofw_init(void (*entry)(void *))
{
	ofw_entry = entry;

	/* XXX Now do stuff.  */
	ofw_console_init();
}

void
ofw_enter(void *args)
{
	ofw_entry(args);
}
