#include <core/types.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <db/db.h>
#include <db/db_show.h>
#include <io/console/console.h>

DB_SHOW_TREE(scheduler, scheduler);
DB_SHOW_VALUE_TREE(scheduler, root, DB_SHOW_TREE_POINTER(scheduler));

static TAILQ_HEAD(, struct scheduler_queue) scheduler_queue_list;
static struct scheduler_queue scheduler_sleep_queue;
static struct spinlock scheduler_lock = SPINLOCK_INIT("SCHEDULER");

static struct pool scheduler_queue_pool = POOL_INIT("SCHEDULER QUEUE",
						    struct scheduler_queue,
						    POOL_VIRTUAL);

#define	SCHEDULER_LOCK()	spinlock_lock(&scheduler_lock)
#define	SCHEDULER_UNLOCK()	spinlock_unlock(&scheduler_lock);

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

#define	current_runq()	PCPU_GET(runq)

static struct scheduler_entry *scheduler_pick_entry(struct scheduler_queue *);
static struct thread *scheduler_pick_thread(void);
static struct scheduler_queue *scheduler_pick_queue(struct scheduler_entry *);
static void scheduler_queue(struct scheduler_queue *, struct scheduler_entry *);
static void scheduler_queue_insert(struct scheduler_queue *, struct scheduler_entry *);
static void scheduler_queue_setup(struct scheduler_queue *, const char *, cpu_id_t);
static void scheduler_switch(struct thread *);

void
scheduler_cpu_pin(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	ASSERT((se->se_flags & SCHEDULER_RUNNING) == 0,
	       "Can't pin running thread.");
	ASSERT((se->se_flags & SCHEDULER_PINNED) == 0,
	       "Can't pin thread twice.");
	se->se_flags |= SCHEDULER_PINNED;
	se->se_oncpu = mp_whoami();
	SCHEDULER_UNLOCK();
}

struct scheduler_queue *
scheduler_cpu_setup(void)
{
	struct scheduler_queue *sq;

	sq = pool_allocate(&scheduler_queue_pool);
	scheduler_queue_setup(sq, "CPU RUN QUEUE", mp_whoami());
	return (sq);
}

void
scheduler_cpu_switchable(void)
{
	struct scheduler_queue *sq = current_runq();

	SCHEDULER_LOCK();
	SQ_LOCK(sq);
	ASSERT(!sq->sq_switchable, "Queue can only be marked switchable once.");
	sq->sq_switchable = true;
	SQ_UNLOCK(sq);
	SCHEDULER_UNLOCK();
}

void
scheduler_init(void)
{
	TAILQ_INIT(&scheduler_queue_list);
	scheduler_queue_setup(&scheduler_sleep_queue, "SCHEDULER SLEEP QUEUE", CPU_ID_INVALID);
}

void
scheduler_schedule(struct spinlock *lock)
{
	struct thread *td;

	SCHEDULER_LOCK();
	if (lock != NULL)
		spinlock_unlock(lock);

	ASSERT(current_thread() == NULL || current_runq()->sq_switchable,
	       "Must be starting first thread or done with initialization.");

	/*
	 * Make sure this thread is bumped to the tail of its queue.
	 */
	if (current_thread() != NULL) {
		struct scheduler_entry *se = &current_thread()->td_sched;
		ASSERT(se->se_queue == current_runq() ||
		       se->se_queue == &scheduler_sleep_queue,
		       "Current thread must be running or sleeping.");
		if (se->se_queue != &scheduler_sleep_queue)
			scheduler_queue(current_runq(), se);
	}

	/*
	 * Find a thread to run.  There should always be something on the run
	 * queue (the idle thread should always be runnable.)
	 */
	td = scheduler_pick_thread();
	if (td == NULL) {
		td = current_thread();
		ASSERT(td != NULL,
		       "Must have a thread to return to in idle system.");
		ASSERT((td->td_sched.se_flags & SCHEDULER_RUNNABLE) != 0,
		       "Thread must be runnable.");
		/* XXX
		 * This should only happen if we are the idle thread and
		 * the system is staying idle!
		 */
	}
	scheduler_switch(td);
}

void
scheduler_thread_runnable(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	se->se_flags |= SCHEDULER_RUNNABLE;
	se->se_flags &= ~SCHEDULER_SLEEPING;
	scheduler_queue(NULL, se);
	ASSERT(se->se_queue != &scheduler_sleep_queue,
	       "Queue should have changed!");
	SCHEDULER_UNLOCK();
}

void
scheduler_thread_setup(struct thread *td)
{
	struct scheduler_entry *se;

	se = &td->td_sched;
	se->se_thread = td;
	se->se_queue = NULL;
	se->se_flags = SCHEDULER_DEFAULT;
	se->se_oncpu = CPU_ID_INVALID;

	/*
	 * Caller must eventually call scheduler_thread_runnable(td).
	 */
}

void
scheduler_thread_sleeping(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	se->se_flags &= ~SCHEDULER_RUNNABLE;
	se->se_flags |= SCHEDULER_SLEEPING;
	SQ_LOCK(&scheduler_sleep_queue);
	scheduler_queue(&scheduler_sleep_queue, se);
	SQ_UNLOCK(&scheduler_sleep_queue);
	SCHEDULER_UNLOCK();
}

static struct scheduler_entry *
scheduler_pick_entry(struct scheduler_queue *sq)
{
	struct scheduler_entry *se, *winner;

	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	SQ_LOCK(sq);

	winner = NULL;
	TAILQ_FOREACH(se, &sq->sq_queue, se_link) {
		if ((se->se_flags & SCHEDULER_RUNNABLE) == 0 ||
		    (se->se_flags & SCHEDULER_RUNNING) != 0)
			continue;
		winner = se;
		break;
	}
	if (winner == NULL) {
		/* XXX migrate? */
		SQ_UNLOCK(sq);
		return (NULL);
	}
	se = winner;

#ifndef	NO_ASSERT
	struct thread *td2 = (struct thread *)((uintptr_t)se -
			      (uintptr_t)&(((struct thread *)NULL)->td_sched));
#endif
	ASSERT(td2 == se->se_thread,
	       "Scheduler entry must point to its own thread.");
	ASSERT(se->se_oncpu == CPU_ID_INVALID || se->se_oncpu == mp_whoami(),
	       "Thread must be pinned to me or no one at all.");

	/*
	 * Push to tail of queue.
	 */
	scheduler_queue(sq, se);

	SQ_UNLOCK(sq);

	return (se);
}

static struct thread *
scheduler_pick_thread(void)
{
	struct scheduler_entry *se;

	se = scheduler_pick_entry(current_runq());
	if (se == NULL)
		return (NULL);
	return (se->se_thread);
}

static struct scheduler_queue *
scheduler_pick_queue(struct scheduler_entry *se)
{
	struct scheduler_queue *sq, *winner;

	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	ASSERT((se->se_flags & SCHEDULER_SLEEPING) == 0,
	       "Cannot pick queue for sleeping thread.");
	if ((se->se_flags & SCHEDULER_PINNED) != 0) {
		TAILQ_FOREACH(sq, &scheduler_queue_list, sq_link) {
			if (sq->sq_cpu == se->se_oncpu) {
				return (sq);
			}
		}
		panic("%s: thread must be pinned to a real queue.", __func__);
	}
	if ((se->se_flags & SCHEDULER_RUNNING) != 0) {
		/*
		 * Always return the current runq here.  A running thread may
		 * have been put on the sleep queue, but this is never called
		 * for a sleeping thread.  If a running thread is taken off the
		 * sleep queue, this will be called, and we need to pick a run
		 * queue to put it on.  Since a running thread cannot migrate,
		 * we thus handle both cases by returning the current runq
		 * here.
		 *
		 * (Note that since we may be doing this from another CPU, we
		 * want the runq that the thread is currently on, not the
		 * current_runq(), which is this CPU's.)
		 */
		ASSERT(se->se_queue != NULL, "Running thread needs a queue.");
		TAILQ_FOREACH(sq, &scheduler_queue_list, sq_link) {
			if (sq->sq_cpu == se->se_oncpu) {
				return (sq);
			}
		}
		ASSERT(false, "Should not be reached.");
	}
	winner = NULL;
	TAILQ_FOREACH(sq, &scheduler_queue_list, sq_link) {
		SQ_LOCK(sq);
		/*
		 * Don't use any queues other than this CPU's unless the CPU
		 * has marked its queue as switchable.
		 */
		if (!sq->sq_switchable && sq != current_runq()) {
			SQ_UNLOCK(sq);
			continue;
		}
		if (winner == NULL) {
			winner = sq;
			continue;
		}
		if (winner->sq_length < sq->sq_length) {
			SQ_UNLOCK(sq);
			continue;
		}
		SQ_UNLOCK(winner);
		winner = sq;
	}
	ASSERT(winner != NULL, "Must find a winner.");
	SQ_UNLOCK(winner);
	return (winner);
}

static void
scheduler_queue(struct scheduler_queue *sq, struct scheduler_entry *se)
{
	struct scheduler_queue *osq;

	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	if (sq == NULL) {
		sq = scheduler_pick_queue(se);
		ASSERT(sq != &scheduler_sleep_queue,
		       "Sleep queue should never be picked.");
	}

	/*
	 * We allow for se->se_queue == sq.  In that case, the se will be
	 * pushed to the end of sq.
	 */
	if (se->se_queue != NULL) {
		osq = se->se_queue;
		SQ_LOCK(osq);
		TAILQ_REMOVE(&osq->sq_queue, se, se_link);
		se->se_queue = NULL;
		osq->sq_length--;
		SQ_UNLOCK(osq);
	}
	scheduler_queue_insert(sq, se);
}

static void
scheduler_queue_insert(struct scheduler_queue *sq, struct scheduler_entry *se)
{
	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	if (sq == &scheduler_sleep_queue) {
		if ((se->se_flags & SCHEDULER_SLEEPING) == 0)
			panic("%s: can't put %p on sleep queue.", __func__, se);
	} else {
		if ((se->se_flags & SCHEDULER_RUNNABLE) == 0)
			panic("%s: can't put %p on run queue.", __func__, se);
	}
	SQ_LOCK(sq);
	se->se_queue = sq;
	TAILQ_INSERT_TAIL(&sq->sq_queue, se, se_link);
	sq->sq_length++;
	SQ_UNLOCK(sq);
}

static void
scheduler_queue_setup(struct scheduler_queue *sq, const char *lk, cpu_id_t cpu)
{
	spinlock_init(&sq->sq_lock, lk);
	SCHEDULER_LOCK();
	SQ_LOCK(sq);
	sq->sq_cpu = cpu;
	TAILQ_INIT(&sq->sq_queue);
	sq->sq_length = 0;
	sq->sq_switchable = false;
	if (sq != &scheduler_sleep_queue)
		TAILQ_INSERT_HEAD(&scheduler_queue_list, sq, sq_link);
	SQ_UNLOCK(sq);
	SCHEDULER_UNLOCK();
}

static void
scheduler_switch(struct thread *td)
{
	struct scheduler_entry *se;
	struct thread *otd;

	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	otd = current_thread();
	se = &td->td_sched;

	ASSERT((se->se_flags & SCHEDULER_SLEEPING) == 0,
	       "Cannot switch to sleeping thread.");
	ASSERT((se->se_flags & SCHEDULER_RUNNABLE) != 0,
	       "Cannot switch to non-runnable thread.");

	if (otd != NULL) {
		struct scheduler_entry *ose;

		ose = &otd->td_sched;

		/*
		 * Clear the on CPU field if the thread is not pinned.
		 */
		if ((ose->se_flags & SCHEDULER_PINNED) == 0)
			ose->se_oncpu = CPU_ID_INVALID;

		ose->se_flags &= ~SCHEDULER_RUNNING;
		/*
		 * If the thread isn't asleep, throw it back onto a runqueue.
		 * This will retain the current runqueue if the thread is
		 * pinned.
		 */
		if (ose->se_queue != &scheduler_sleep_queue)
			scheduler_queue(NULL, ose);
	}
	if ((se->se_flags & SCHEDULER_PINNED) != 0) {
		if (se->se_oncpu != mp_whoami())
			panic("%s: cannot migrate pinned thread.", __func__);
	}
	se->se_flags |= SCHEDULER_RUNNING;
	se->se_flags &= ~SCHEDULER_RUNNABLE;
	se->se_oncpu = mp_whoami();
	SCHEDULER_UNLOCK();
	if (otd != td)
		thread_switch(otd, td);
}

static void
scheduler_db_dump_queue(struct scheduler_queue *sq)
{
	struct scheduler_entry *se;

	kcprintf("queue %p cpu%u length %u %s\n",
		 sq, sq->sq_cpu, sq->sq_length, (sq->sq_switchable ?
						 "switchable" : "!switchable"));
	TAILQ_FOREACH(se, &sq->sq_queue, se_link) {
		struct thread *td;

		td = se->se_thread;

		kcprintf("%p (thread %p, \"%s\")%s%s%s%s cpu%u\n",
			 se, td, td->td_name,
			 ((se->se_flags & SCHEDULER_RUNNING) ?
			  " running" : ""),
			 ((se->se_flags & SCHEDULER_PINNED) ?
			  " pinned" : ""),
			 ((se->se_flags & SCHEDULER_SLEEPING) ?
			  " sleeping" : ""),
			 ((se->se_flags & SCHEDULER_RUNNABLE) ?
			  " runnable" : ""),
			 se->se_oncpu);
	}
}

static void
scheduler_db_dump(void)
{
	struct scheduler_queue *sq;

	kcprintf("Dumping scheduler queues...\n");
	TAILQ_FOREACH(sq, &scheduler_queue_list, sq_link)
		scheduler_db_dump_queue(sq);
	kcprintf("Done.\n");
	kcprintf("Dumping sleep queue...\n");
	scheduler_db_dump_queue(&scheduler_sleep_queue);
	kcprintf("Done.\n");
}
DB_SHOW_VALUE_VOIDF(queues, scheduler, scheduler_db_dump);
