#include <core/types.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_console.h>

static ofw_return_t (*ofw_entry)(void *);

void
ofw_init(ofw_return_t (*entry)(void *))
{
	ofw_entry = entry;

	/* XXX Now do stuff.  */
	ofw_console_init();
}

ofw_return_t
ofw_call(void *args)
{
	return (ofw_entry(args));
}
