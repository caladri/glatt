#include <core/types.h>
#include <db/db.h>
#include <db/db_action.h>
#include <db/db_show.h>
#include <io/device/console/console.h>

SET(db_show_trees, struct db_show_tree);
SET(db_show_values, struct db_show_value);

static void
db_show(void)
{
	panic("Unimplemented!");
}
DB_ACTION(s, "Explore a maze of twisty values!", db_show);
