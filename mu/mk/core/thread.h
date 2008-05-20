#ifndef	_CORE_THREAD_H_
#define	_CORE_THREAD_H_

#ifndef	NO_MORDER
#include <core/morder.h>
#endif
#include <core/queue.h>
#include <core/scheduler.h>
#include <cpu/context.h>
#include <cpu/pcpu.h>
#include <cpu/thread.h>

#define	THREAD_NAME_SIZE	(128)

#define	THREAD_DEFAULT	(0x00000000)	/* Default thread flags.  */

struct thread {
	struct task *td_parent;
	STAILQ_ENTRY(struct thread) td_link;
	char td_name[THREAD_NAME_SIZE];
	unsigned td_flags;
#ifndef	NO_MORDER
	struct morder_thread td_morder;
#endif
	struct scheduler_entry td_sched;
	struct context td_context;	/* State for context switching.  */
	void *td_kstack;
	struct cpu_thread td_cputhread;
};

void thread_init(void);

int thread_create(struct thread **, struct task *, const char *, unsigned);
void thread_set_upcall(struct thread *, void (*)(void *), void *);
void thread_switch(struct thread *, struct thread *);
void thread_trampoline(struct thread *, void (*)(void *), void *);

#endif /* !_CORE_THREAD_H_ */
