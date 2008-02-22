#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>

#include <io/device/console/console.h>

static void
idle_thread(void *arg)
{
	for (;;) {
		scheduler_thread_runnable(current_thread());
		scheduler_schedule();
	}
}

static void
idle_thread_startup(void *arg)
{
	struct thread *td;
	struct task *task;
	int error;

	error = task_create(&task, NULL, "idle task",
			    TASK_DEFAULT | TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&td, task, "idle thread", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);
	thread_set_upcall(td, idle_thread, NULL);
	scheduler_thread_runnable(td);
}
STARTUP_ITEM(idle_thread, STARTUP_IDLE, STARTUP_CPU, idle_thread_startup, NULL);
