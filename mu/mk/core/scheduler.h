#ifndef	_CORE_SCHEDULER_H_
#define	_CORE_SCHEDULER_H_

#include <core/queue.h>
#include <core/spinlock.h>

struct scheduler_queue;
struct thread;

#define	SCHEDULER_DEFAULT	(0x00000000)	/* Default flags.  */
#define	SCHEDULER_RUNNING	(0x00000001)	/* Thread is running.  */
#define	SCHEDULER_PINNED	(0x00000002)	/* Thread is pinned to CPU.  */
#define	SCHEDULER_MAIN		(0x00000004)	/* Thread is main thread.  */

struct scheduler_entry {
	struct thread *se_thread;
	struct scheduler_queue *se_queue;
	TAILQ_ENTRY(struct scheduler_entry) se_link;
	unsigned se_flags;
	cpu_id_t se_oncpu;
};

struct scheduler_queue {
	struct spinlock sq_lock;
	cpu_id_t sq_cpu;
	TAILQ_HEAD(, struct scheduler_entry) sq_queue;
	unsigned sq_length;
	TAILQ_ENTRY(struct scheduler_queue) sq_link;
};

void scheduler_cpu_main(struct thread *);
void scheduler_cpu_pin(struct thread *);
void scheduler_cpu_setup(struct scheduler_queue *);
void scheduler_init(void);
void scheduler_schedule(void);
void scheduler_thread_runnable(struct thread *);
void scheduler_thread_setup(struct thread *);
void scheduler_thread_sleeping(struct thread *);

#endif /* _CORE_SCHEDULER_H_ */
