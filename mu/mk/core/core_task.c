#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <db/db_command.h>
#include <io/device/console/console.h>
#include <vm/page.h>

/*
 * Determines which allocator we use.  We can switch to vm_alloc if the task
 * structure gets bigger than a PAGE.
 */
COMPILE_TIME_ASSERT(sizeof (struct task) <= PAGE_SIZE);

static struct pool task_pool = POOL_INIT("TASK", struct task, POOL_VIRTUAL);
static STAILQ_HEAD(, struct task) task_list = STAILQ_HEAD_INITIALIZER(task_list);

int
task_create(struct task **taskp, struct task *parent, const char *name,
	    uint32_t flags)
{
	struct task *task;
	int error;

	task = pool_allocate(&task_pool);
	if (task == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(task->t_name, name, sizeof task->t_name);
	task->t_parent = parent;
	STAILQ_INIT(&task->t_children);
	STAILQ_INIT(&task->t_threads);
	if (parent != NULL) {
		STAILQ_INSERT_TAIL(&task_list, task, t_link);
	} else {
		STAILQ_INSERT_TAIL(&parent->t_children, task, t_link);
	}
	task->t_flags = flags;
	/*
	 * CPU task setup takes care of:
	 * 	vm
	 */
	error = cpu_task_setup(task);
	if (error != 0) {
		panic("%s: need to destroy task, cpu_task_setup failed: %m",
		      __func__, error);
		return (error);
	}
	*taskp = task;
	return (0);
}
