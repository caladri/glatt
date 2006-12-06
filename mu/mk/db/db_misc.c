#include <core/types.h>
#include <core/startup.h>
#include <db/db.h>
#include <db/db_action.h>
#include <io/device/console/console.h>

static void
db_halt(void)
{
	kcprintf("%s: Halting...\n", __func__);
	platform_halt();
}
DB_ACTION(h, "Halt the system.", db_halt);

static void
db_panic(void)
{
	panic("%s: called", __func__);
}
DB_ACTION(p, "Panic the system.", db_panic);
