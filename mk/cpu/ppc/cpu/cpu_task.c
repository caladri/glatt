#include <core/types.h>
#include <core/task.h>
#include <cpu/segment.h>
#include <vm/vm.h>

void
cpu_task_free(struct task *task)
{
	if ((task->t_flags & TASK_KERNEL) == 0)
		vm_exit(task->t_vm);
}

int
cpu_task_setup(struct task *task)
{
	int error;

	if ((task->t_flags & TASK_KERNEL) == 0) {
		error = vm_setup(&task->t_vm, USER_BASE, USER_END);
		if (error != 0)
			return (error);
	}
	return (0);
}
