#ifndef	_CORE_TASK_H_
#define	_CORE_TASK_H_

#include <cpu/pcpu.h>

struct vm;

struct task {
	struct vm *t_vm;
};

#endif /* !_CORE_TASK_H_ */
