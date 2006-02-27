#ifndef	_CORE_TASK_H_
#define	_CORE_TASK_H_

#include <cpu/pcpu.h>
#include <vm/vm.h>

#define	TASK_DEFAULT	(0x00000000)	/* Default task flags.  */

struct task {
	const char *t_name;
	uint32_t t_flags;
	struct vm t_vm;
};

int task_create(struct task **, const char *, uint32_t);

#endif /* !_CORE_TASK_H_ */
