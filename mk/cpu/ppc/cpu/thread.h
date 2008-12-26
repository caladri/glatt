#ifndef	_CPU_THREAD_H_
#define	_CPU_THREAD_H_

struct thread;

#define	KSTACK_SIZE	(2 * PAGE_SIZE)
#define	MAILBOX_SIZE	(1 * PAGE_SIZE)

#define	current_thread()	PCPU_GET(thread)

struct cpu_thread {
	vaddr_t td_mbox;
};

void cpu_thread_set_upcall(struct thread *, void (*)(void *), void *);
int cpu_thread_setup(struct thread *);

#endif /* !_CPU_THREAD_H_ */
