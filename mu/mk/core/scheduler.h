#ifndef	_CORE_SCHEDULER_H_
#define	_CORE_SCHEDULER_H_

#include <core/queue.h>
#include <core/spinlock.h>

struct scheduler_queue;
struct thread;

#define	SCHEDULER_DEFAULT	(0x00000000)	/* Default flags.  */
#define	SCHEDULER_RUNNING	(0x00000001)	/* Thread is running.  */
#define	SCHEDULER_PINNED	(0x00000002)	/* Thread is pinned to CPU.  */
#define	SCHEDULER_SLEEPING	(0x00000004)	/* Thread is sleeping.  */
#define	SCHEDULER_RUNNABLE	(0x00000008)	/* Thread is runnable.  */

struct scheduler_entry {
	struct thread *se_thread;
	TAILQ_ENTRY(struct scheduler_entry) se_link;
	unsigned se_flags;
	cpu_id_t se_oncpu;
};

void scheduler_cpu_pin(struct thread *);
struct scheduler_queue *scheduler_cpu_setup(void);
void scheduler_init(void);
void scheduler_schedule(struct thread *, struct spinlock *);
void scheduler_thread_runnable(struct thread *);
void scheduler_thread_setup(struct thread *);
void scheduler_thread_sleeping(struct thread *);

#endif /* _CORE_SCHEDULER_H_ */
