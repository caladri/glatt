#ifndef	_IPC_TASK_H_
#define	_IPC_TASK_H_

#include <core/queue.h>

struct ipc_port_right;

struct ipc_task {
	STAILQ_HEAD(, struct ipc_port_right) ipct_rights;
};

void ipc_task_free(struct task *) __non_null(1);
int ipc_task_setup(struct task *) __non_null(1) __check_result;

#endif /* !_IPC_TASK_H_ */
