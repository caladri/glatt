#ifndef	_CORE_TASK_H_
#define	_CORE_TASK_H_

#include <core/queue.h>
#include <cpu/pcpu.h>
#include <cpu/task.h>
#include <vm/vm.h>

#define	TASK_NAME_SIZE	(128)

#define	TASK_DEFAULT	(0x00000000)	/* Default task flags.  */
#define	TASK_KERNEL	(0x00000001)	/* Run in kernel address space.  */

struct task {
	char t_name[TASK_NAME_SIZE];
	struct task *t_parent;
	STAILQ_HEAD(, struct task) t_children;
	STAILQ_HEAD(, struct thread) t_threads;
	STAILQ_ENTRY(struct task) t_link;
	uint32_t t_flags;
	struct vm *t_vm;
};

int task_create(struct task **, struct task *, const char *, uint32_t);

#endif /* !_CORE_TASK_H_ */
