#include <core/types.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <db/db.h>
#include <io/device/console/console.h>

SET(startup_items, struct startup_item);

static bool startup_booting = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");

void
startup_boot(void)
{
	struct startup_item **itemp, *item;
	kcprintf("The system is coming up.\n");
	for (itemp = SET_BEGIN(startup_items); itemp < SET_END(startup_items);
	     itemp++) {
		item = *itemp;
		/*
		 * XXX
		 * Sort items.
		 */
		item->si_function(item->si_arg);
	}
}

void
startup_main(void)
{
	/*
	 * XXX
	 * Create thread for this CPU and switch to it if we aren't the
	 */
	spinlock_lock(&startup_spinlock);
	if (!startup_booting) {
		startup_booting = true;
		spinlock_unlock(&startup_spinlock);
		startup_boot();
	} else {
		spinlock_unlock(&startup_spinlock);
	}
	for (;;) {
	}
}
