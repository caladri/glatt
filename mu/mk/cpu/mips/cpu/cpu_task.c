#include <core/types.h>
#include <core/error.h>
#include <core/task.h>
#include <vm/page.h>
#include <vm/vm.h>

int
cpu_task_setup(struct task *task)
{
	int error;

	if ((task->t_flags & TASK_KERNEL) == TASK_KERNEL) {
		task->t_vm = &kernel_vm;
	} else {
		error = vm_setup(&task->t_vm, USER_BASE, USER_END);
		if (error != 0)
			return (error);
	}
	return (0);
}
