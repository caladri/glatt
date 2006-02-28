#include <core/types.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <db/db.h>
#include <io/device/console/console.h>

static bool startup_booted = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");

void
startup_boot(void)
{
	kcprintf("The system is coming up.\n");
	startup_booted = true;
}

void
startup_main(void)
{
	spinlock_lock(&startup_spinlock);
	if (!startup_booted) {
		startup_boot();
		panic("%s: nothing to do.", __func__);
	}
	spinlock_unlock(&startup_spinlock);
	for (;;) {
	}
}
