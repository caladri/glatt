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

static struct task *task_main;
static struct pool task_pool = POOL_INIT("TASK", struct task, POOL_VIRTUAL);

int
task_create(struct task **taskp, struct task *parent, const char *name,
	    uint32_t flags)
{
	struct task *task;
	int error;

	task = pool_allocate(&task_pool);
	if (task == NULL)
		return (ERROR_EXHAUSTED);
	if (parent == NULL) {
		if (task_main == NULL)
			task_main = task;
		else
			parent = task_main;
	}
	strlcpy(task->t_name, name, sizeof task->t_name);
	task->t_parent = parent;
	task->t_children = NULL;
	task->t_threads = NULL;
	task->t_next = NULL;
	if (parent != NULL) {
		task->t_next = parent->t_children;
		parent->t_children = task;
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

/*
 * XXX Should move thread stuff somewhere else?  Need a way to export task_main.
 */
#include <core/thread.h>

static void
task_db_dump_thread(struct thread *td)
{
	kcprintf("\tTHREAD %p ", (void *)td);
	if ((td->td_flags & THREAD_BLOCKED) != 0)
		kcprintf("B");
	else
		kcprintf("-");
	if ((td->td_flags & THREAD_RUNNING) != 0)
		kcprintf("R");
	else
		kcprintf("-");
	if ((td->td_flags & THREAD_PINNED) != 0)
		kcprintf("P");
	else
		kcprintf("-");
	if (td->td_oncpu != CPU_ID_INVALID)
		kcprintf(" (CPU %u)", td->td_oncpu);
	kcprintf(" [%s]\n", td->td_name);
}

static void
task_db_dump_threads(struct task *task)
{
	struct thread *td;

	for (td = task->t_threads; td != NULL; td = td->td_next)
		task_db_dump_thread(td);
}

static void
task_db_dump_tree(struct task *task)
{
	struct task *child;

	if (task == NULL)
		return;
	kcprintf("TASK %p => %s\n", (void *)task, task->t_name);
	task_db_dump_threads(task);
	for (child = task->t_children; child != NULL; child = child->t_next)
		task_db_dump_tree(child);
}

static void
task_db_dump(void)
{
	kcprintf("Beginning task dump...\n");
	task_db_dump_tree(task_main);
	kcprintf("Finished.\n");
}
DB_COMMAND(task_dump, task_db_dump, "Dump all tasks.");
