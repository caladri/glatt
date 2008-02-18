#ifndef	_CORE_THREAD_H_
#define	_CORE_THREAD_H_

#include <core/morder.h>
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
	uint32_t td_flags;
	struct morder_thread td_morder;
	struct scheduler_entry td_sched;
	struct context td_context;	/* State for context switching.  */
	void *td_kstack;
	struct cpu_thread td_cputhread;
};

int thread_create(struct thread **, struct task *, const char *, uint32_t);
void thread_set_upcall(struct thread *, void (*)(void *), void *);
void thread_switch(struct thread *, struct thread *);
void thread_trampoline(struct thread *, void (*)(void *), void *);

#endif /* !_CORE_THREAD_H_ */
