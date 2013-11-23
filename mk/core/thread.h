#ifndef	_CORE_THREAD_H_
#define	_CORE_THREAD_H_

#include <core/queue.h>
#include <core/scheduler.h>
#include <cpu/context.h>
#include <cpu/pcpu.h>
#include <cpu/thread.h>

#define	THREAD_NAME_SIZE	(128)

#define	THREAD_DEFAULT	(0x00000000)	/* Default thread flags.  */
#define	THREAD_USTACK	(0x00000001)	/* Thread has user stack.  */

struct thread {
	struct task *td_task;
	STAILQ_ENTRY(struct thread) td_link;
	char td_name[THREAD_NAME_SIZE];
	unsigned td_flags;
	struct scheduler_entry td_sched;
	struct context td_context;	/* State for context switching.  */
	vaddr_t td_kstack;
	vaddr_t td_ustack_bottom;
	vaddr_t td_ustack_top;
	struct cpu_thread td_cputhread;
};

void thread_init(void);

int thread_create(struct thread **, struct task *, const char *, unsigned) __non_null(1, 2, 3) __check_result;
void thread_exit(void) __noreturn;
void thread_free(struct thread *) __non_null(1);
void thread_set_upcall(struct thread *, void (*)(struct thread *, void *), void *) __non_null(1, 2);
void thread_set_upcall_user(struct thread *, vaddr_t, register_t) __non_null(1);
void thread_switch(struct thread *, struct thread *) __non_null(2);

#endif /* !_CORE_THREAD_H_ */
