#ifndef	_CORE_TASK_H_
#define	_CORE_TASK_H_

#include <core/queue.h>
#include <cpu/pcpu.h>
#include <cpu/task.h>
#include <ipc/ipc.h>
#include <ipc/task.h>

struct vm;

#define	TASK_NAME_SIZE	(128)

#define	TASK_DEFAULT	(0x00000000)	/* Default task flags.  */
#define	TASK_KERNEL	(0x00000001)	/* Run in kernel address space.  */

struct task {
	char t_name[TASK_NAME_SIZE];
	STAILQ_HEAD(, struct thread) t_threads;
	STAILQ_ENTRY(struct task) t_link;
	unsigned t_flags;
	struct vm *t_vm;
	struct ipc_task t_ipc;
};

void task_init(void);

int task_create(struct task **, const char *, unsigned) __non_null(1) __check_result;
void task_free(struct task *) __non_null(1);

#endif /* !_CORE_TASK_H_ */
