#include <core/types.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>

#include <io/device/console/console.h>

static unsigned test_ball;

static void
test_thread_player(void *arg)
{
	unsigned i;

	i = (unsigned)(uintptr_t)arg;

	kcprintf("Player %u playing...\n", i);

	for (;;) {
		kcprintf("%u got the ball on cpu%u!\n", i, mp_whoami());
		if (i == 0) {
			kcprintf("(waking everyone up cause I'm a troublemaker!)\n");
			sleepq_signal(&test_ball);
		} else {
			kcprintf("(waking up the next person in queue.)\n");
			sleepq_signal_one(&test_ball);
		}
		sleepq_wait(&test_ball);
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
		thread_set_upcall(td, test_thread_player, (void *)(uintptr_t)i);
		scheduler_thread_runnable(td);
	}
}
STARTUP_ITEM(test_thread, STARTUP_MAIN, STARTUP_BEFORE, test_thread_startup, NULL);
