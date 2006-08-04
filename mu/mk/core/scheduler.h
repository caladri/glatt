#ifndef	_CORE_SCHEDULER_H_
#define	_CORE_SCHEDULER_H_

#include <core/spinlock.h>

struct scheduler_queue;
struct thread;

#define	SCHEDULER_DEFAULT	(0x00000000)	/* Default flags.  */
#define	SCHEDULER_RUNNING	(0x00000001)	/* Thread is running.  */
#define	SCHEDULER_PINNED	(0x00000002)	/* Thread is pinned to CPU.  */
#define	SCHEDULER_IDLE		(0x00000004)	/* Thread is idle thread.  */

struct scheduler_entry {
	struct scheduler_entry *se_next;
	struct thread *se_thread;
	struct scheduler_queue *se_queue;
	struct scheduler_entry *se_prev;
	unsigned se_flags;
	cpu_id_t se_oncpu;
};

struct scheduler_queue {
	struct spinlock sq_lock;
	cpu_id_t sq_cpu;
	struct scheduler_entry *sq_first;
	struct scheduler_entry *sq_last;
	struct scheduler_queue *sq_link;
	unsigned sq_length;
};

void scheduler_cpu_idle(struct thread *);
void scheduler_cpu_pin(struct thread *);
void scheduler_cpu_setup(struct scheduler_queue *);
void scheduler_schedule(void);
void scheduler_thread_runnable(struct thread *);
void scheduler_thread_setup(struct thread *);

#endif /* _CORE_SCHEDULER_H_ */
