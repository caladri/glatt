#ifndef	_CORE_SCHEDULER_H_
#define	_CORE_SCHEDULER_H_

#include <core/queue.h>

struct spinlock;
struct thread;

#define	SCHEDULER_DEFAULT	(0x00000000)	/* Default flags.  */
#define	SCHEDULER_RUNNING	(0x00000001)	/* Thread is running.  */
#ifndef UNIPROCESSOR
#define	SCHEDULER_PINNED	(0x00000002)	/* Thread is pinned to CPU.  */
#endif
#define	SCHEDULER_SLEEPING	(0x00000004)	/* Thread is sleeping.  */
#define	SCHEDULER_RUNNABLE	(0x00000008)	/* Thread is runnable.  */
#define	SCHEDULER_EXITING	(0x00000010)	/* Thread is exiting.  */

struct scheduler_entry {
	struct thread *se_thread;
	TAILQ_ENTRY(struct scheduler_entry) se_link;
	unsigned se_flags;
#ifndef UNIPROCESSOR
	cpu_id_t se_oncpu;
#endif
};

void scheduler_init(void);

void scheduler_activate(struct thread *) __non_null(1);
void scheduler_cpu_pin(struct thread *) __non_null(1);
void scheduler_schedule(struct thread *, struct spinlock *);
void scheduler_thread_exiting(void);
void scheduler_thread_free(struct thread *) __non_null(1);
void scheduler_thread_runnable(struct thread *) __non_null(1);
void scheduler_thread_setup(struct thread *) __non_null(1);
void scheduler_thread_sleeping(struct thread *) __non_null(1);

#endif /* !_CORE_SCHEDULER_H_ */
