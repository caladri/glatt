#ifndef	_CPU_TASK_H_
#define	_CPU_TASK_H_

struct task;

#define	current_task()							\
	(current_thread() == NULL ? NULL : current_thread()->td_task)

int cpu_task_setup(struct task *) __non_null(1);

#endif /* !_CPU_TASK_H_ */
