#include <core/types.h>
#include <core/error.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/task.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/task.h>

void
ipc_task_free(struct task *task)
{
	/*
	 * XXX
	 * Needs to be implemented.
	 */
}

int
ipc_task_setup(ipc_port_t parent, struct task *task)
{
	struct ipc_task *ipct = &task->t_ipc;
	int error;

	STAILQ_INIT(&ipct->ipct_rights);

	/*
	 * If this is a kernel task, do not allocate a task port.
	 */
	if ((task->t_flags & TASK_KERNEL) != 0) {
		if (parent != IPC_PORT_UNKNOWN)
			panic("%s: kernel task must not have parent IPC port.", __func__);
		return (0);
	}

	/*
	 * Allocate a task port.
	 */
	error = ipc_port_allocate(&ipct->ipct_task_port, IPC_PORT_FLAG_DEFAULT);
	if (error != 0)
		return (error);

	if (parent != IPC_PORT_UNKNOWN) {
		/*
		 * Grant a send right to the parent.
		 */
		error = ipc_port_right_send(parent, ipct->ipct_task_port, IPC_PORT_RIGHT_SEND);
		if (error != 0)
			panic("%s: ipc_port_right_grant (send) failed: %m", __func__, error);

		/*
		 * XXX
		 * We should cache the parent port here, and have some kind of hooks
		 * so that we clear it if it closes, etc., to prevent reuse issues.
		 */
	}

	/*
	 * Grant a receive right to the task itself.
	 */
	error = ipc_port_right_grant(task, ipct->ipct_task_port, IPC_PORT_RIGHT_RECEIVE);
	if (error != 0)
		panic("%s: ipc_port_right_grant (receive) failed: %m", __func__, error);

	/*
	 * Drop our receive right.
	 */
	error = ipc_port_right_drop(ipct->ipct_task_port, IPC_PORT_RIGHT_RECEIVE);
	if (error != 0)
		panic("%s: ipc_port_right_drop failed: %m", __func__, error);

	return (0);
}
