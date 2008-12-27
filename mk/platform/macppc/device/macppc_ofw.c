#include <core/types.h>
#include <device/ofw.h>
#include <io/ofw/ofw.h>
#include <platform/startup.h>

static ofw_return_t (*macppc_ofw_entry)(void *);

ofw_return_t
macppc_ofw_call(void *args)
{
	ofw_return_t ret;

	/* XXX Save current MSRs.  */

	/* XXX Restore OFW MSRs.  */

	ret = macppc_ofw_entry(args);

	/* XXX Restore current MSRs.  */

	return (ret);
}

void
platform_ofw_init(uintptr_t ofw_entry)
{
	/* XXX Save OFW MSRs.  */

	macppc_ofw_entry = (ofw_return_t (*)(void *))ofw_entry;
}
