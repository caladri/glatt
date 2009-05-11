#include <core/types.h>
#include <cpu/startup.h>
#include <db/db.h>
#include <db/db_command.h>
#include <io/console/console.h>

static void
db_halt(void)
{
	kcprintf("%s: Halting...\n", __func__);
	cpu_halt();
}
DB_COMMAND(halt, root, db_halt);
DB_COMMAND(quit, root, db_halt);

static void
db_panic(void)
{
	panic("%s: called", __func__);
}
DB_COMMAND(panic, root, db_panic);
