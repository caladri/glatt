#ifndef	_IPC_TASK_H_
#define	_IPC_TASK_H_

#include <core/btree.h>
#include <ipc/ipc.h>

struct ipc_port_right;

struct ipc_task {
	BTREE_ROOT(struct ipc_port_right) ipct_rights;
};

void ipc_task_init(void);

int ipc_task_check_port_right(struct task *, ipc_port_right_t, ipc_port_t);
int ipc_task_insert_port_right(struct task *, ipc_port_right_t, ipc_port_t);
int ipc_task_setup(struct task *);

#endif /* !_IPC_TASK_H_ */