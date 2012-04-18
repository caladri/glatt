#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/string.h>
#include <core/task.h>

static struct pool task_pool;
static STAILQ_HEAD(, struct task) task_list;

void
task_init(void)
{
	int error;

	error = pool_create(&task_pool, "TASK", sizeof (struct task),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	STAILQ_INIT(&task_list);

	ipc_task_init();
}

int
task_create(struct task **taskp, const char *name, unsigned flags)
{
	struct task *task;
	int error;

	task = pool_allocate(&task_pool);
	if (task == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(task->t_name, name, sizeof task->t_name);
	STAILQ_INIT(&task->t_threads);
	STAILQ_INSERT_TAIL(&task_list, task, t_link);
	task->t_flags = flags;
	/*
	 * CPU task setup takes care of:
	 * 	vm
	 */
	error = cpu_task_setup(task);
	if (error != 0) {
		panic("%s: need to destroy task, cpu_task_setup failed: %m",
		      __func__, error);
	}

	error = ipc_task_setup(task);
	if (error != 0) {
		panic("%s: need to destroy task, ipc_task_setup failed: %m",
		      __func__, error);
	}

	*taskp = task;
	return (0);
}

void
task_free(struct task *task)
{
	ASSERT(STAILQ_EMPTY(&task->t_threads), "Freeing task must not have any threads.");

	STAILQ_REMOVE(&task_list, task, struct task, t_link);

	cpu_task_free(task);

	ipc_task_free(task);

	pool_free(task);
}
