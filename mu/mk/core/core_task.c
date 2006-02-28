#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <vm/page.h>

/*
 * Determines which allocator we use.  We can switch to vm_alloc if the task
 * structure gets bigger than a PAGE.
 */
COMPILE_TIME_ASSERT(sizeof (struct task) <= PAGE_SIZE);

static struct pool task_pool = POOL_INIT("TASK", struct task, POOL_VIRTUAL);

int
task_create(struct task **taskp, struct task *parent, const char *name,
	    uint32_t flags)
{
	struct task *task;

	task = pool_allocate(&task_pool);
	if (task == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(task->t_name, name, sizeof task->t_name);
	task->t_parent = parent;
	task->t_children = NULL;
	task->t_next = NULL;
	if (parent != NULL) {
		task->t_next = parent->t_children;
		parent->t_children = task;
	}
	task->t_flags = flags;
	vm_setup(&task->t_vm);
	return (0);
}
