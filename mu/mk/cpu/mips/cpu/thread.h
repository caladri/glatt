#ifndef	_CPU_THREAD_H_
#define	_CPU_THREAD_H_

struct thread;

#define	KSTACK_SIZE	(2 * PAGE_SIZE)

#define	current_thread()	PCPU_GET(thread)

int cpu_thread_setup(struct thread *);

#endif /* !_CPU_THREAD_H_ */
