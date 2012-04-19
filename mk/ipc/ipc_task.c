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
ipc_task_setup(struct task *task)
{
	struct ipc_task *ipct = &task->t_ipc;

	STAILQ_INIT(&ipct->ipct_rights);

	/* Insert any default rights.  */

	return (0);
}
