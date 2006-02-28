#ifndef	_CORE_TASK_H_
#define	_CORE_TASK_H_

#include <cpu/pcpu.h>
#include <vm/vm.h>

#define	TASK_NAME_SIZE	(128)

#define	TASK_DEFAULT	(0x00000000)	/* Default task flags.  */

struct task {
	char t_name[TASK_NAME_SIZE];
	struct task *t_parent;
	struct task *t_children;
	struct task *t_next;
	uint32_t t_flags;
	struct vm t_vm;
};

int task_create(struct task **, struct task *, const char *, uint32_t);

#endif /* !_CORE_TASK_H_ */
