#include <core/types.h>
#include <device/ofw.h>
#include <io/ofw/ofw.h>
#include <platform/startup.h>

static void (*macppc_ofw_entry)(void *);

void
macppc_ofw_enter(void *args)
{
	/* XXX Save current MSRs.  */
	/* XXX Restore OFW MSRs.  */
	macppc_ofw_entry(args);
	/* XXX Restore current MSRs.  */
}

void
platform_ofw_init(uintptr_t ofw_entry)
{
	/* XXX Save OFW MSRs.  */

	macppc_ofw_entry = (void (*)(void *))ofw_entry;
}
