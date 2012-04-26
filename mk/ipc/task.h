#ifndef	_IPC_TASK_H_
#define	_IPC_TASK_H_

#ifdef MK
#include <core/queue.h>

struct ipc_port_right;

struct ipc_task {
	STAILQ_HEAD(, struct ipc_port_right) ipct_rights;
	ipc_port_t ipct_task_port;
};

void ipc_task_free(struct task *) __non_null(1);
int ipc_task_setup(ipc_port_t, struct task *) __non_null(2) __check_result;
#else
ipc_port_t ipc_task_port(void) __check_result;
#endif

#endif /* !_IPC_TASK_H_ */
