#include <core/types.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <io/device/console/console.h>

SET(startup_items, struct startup_item);

static bool startup_booting = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");
static struct task *main_task;

void
startup_boot(void)
{
	struct startup_item **itemp, *item;

	startup_booting = true;
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
	struct thread *thread;
	bool bootstrap;
	int error;

	spinlock_lock(&startup_spinlock);
	bootstrap = !startup_booting;

	if (bootstrap) {
		error = task_create(&main_task, NULL, "main", TASK_DEFAULT);
		if (error != 0)
			panic("%s: task_create failed: %u", __func__, error);
	}

	error = thread_create(&thread, main_task, "", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %u", __func__, error);

	spinlock_unlock(&startup_spinlock);

	/*
	 * XXX
	 * Switch to running our thread.
	 */

	if (bootstrap)
		startup_boot();

	for (;;) {
	}
}
