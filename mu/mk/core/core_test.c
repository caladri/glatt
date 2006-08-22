#include <core/types.h>
#include <core/mutex.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>

#include <io/device/console/console.h>

static struct mutex test_mutex;

static void
test_thread_player(void *arg)
{
	struct mutex *mtx;

	mtx = arg;

	for (;;) {
		mutex_lock(mtx);
		kcprintf("%p got the ball on cpu%u!\n",
			 current_thread(), mp_whoami());
		mutex_unlock(mtx);

		scheduler_schedule(); /* Yield.  No preemption yet.  */
	}
}

/* And then I hear...  */
static void
test_thread_startup(void *arg)
{
#define	NPLAYERS	32
	struct task *task;
	struct thread *td;
	unsigned i;
	int error;

	mutex_init(&test_mutex, "ball");

	kcprintf("Setting up a nice ball game...\n");
	error = task_create(&task, NULL, "ball game", TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);
	for (i = 0; i < NPLAYERS; i++) {
		kcprintf("Created player %u...\n", i);
		error = thread_create(&td, task, "ball player", THREAD_DEFAULT);
		if (error != 0)
			panic("%s: thread_create #%u failed: %m",
			      __func__, i, error);
		thread_set_upcall(td, test_thread_player, &test_mutex);
		scheduler_thread_runnable(td);
	}
}
STARTUP_ITEM(test_thread, STARTUP_MAIN, STARTUP_BEFORE, test_thread_startup, NULL);
