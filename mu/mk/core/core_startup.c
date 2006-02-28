#include <core/types.h>
#include <core/startup.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
startup_boot(void)
{
	kcprintf("The system is coming up.\n");
}

void
startup_main(void)
{
	panic("%s: nothing to do.", __func__);
}
