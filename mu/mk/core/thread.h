#ifndef	_CORE_THREAD_H_
#define	_CORE_THREAD_H_

#include <cpu/context.h>
#include <cpu/frame.h>
#include <cpu/pcpu.h>
#include <cpu/thread.h>

#define	THREAD_NAME_SIZE	(128)

#define	THREAD_DEFAULT	(0x00000000)	/* Default thread flags.  */
#define	THREAD_BLOCKED	(0x00000001)	/* Waiting for a wakeup.  */
#define	THREAD_RUNNING	(0x00000002)	/* Thread is currently running.  */
#define	THREAD_PINNED	(0x00000004)	/* Thread cannot migrate.  */

struct thread {
	struct task *td_parent;
	struct thread *td_next;
	char td_name[THREAD_NAME_SIZE];
	uint32_t td_flags;
	cpu_id_t td_oncpu;		/* Active CPU (if RUNNING).  */
	struct frame td_frame;		/* Frame for interrupts and such.  */
	struct context td_context;	/* State for context switching.  */
	void *td_kstack;
};

void thread_block(void);
int thread_create(struct thread **, struct task *, const char *, uint32_t);
void thread_set_upcall(struct thread *, void (*)(void *), void *);
void thread_switch(struct thread *, struct thread *);
void thread_trampoline(struct thread *, void (*)(void *), void *);
void thread_wakeup(struct thread *);

#endif /* !_CORE_THREAD_H_ */
