#include <core/types.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <cpu/scheduler.h>
#include <db/db.h>
#include <db/db_show.h>
#include <io/device/console/console.h>

DB_SHOW_TREE(scheduler, scheduler, true);

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

static struct scheduler_entry *scheduler_pick_entry(struct scheduler_queue *);
static struct thread *scheduler_pick_thread(void);
static struct scheduler_queue *scheduler_pick_queue(struct scheduler_entry *);
static void scheduler_queue(struct scheduler_queue *, struct scheduler_entry *);
static void scheduler_queue_setup(struct scheduler_queue *, const char *, cpu_id_t);
static void scheduler_switch(struct thread *);
static void scheduler_yield(void);

void
scheduler_cpu_main(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	se->se_flags |= SCHEDULER_MAIN;
	ASSERT(se->se_queue == NULL, "main thread cannot be on a queue.");
	ASSERT(PCPU_GET(maintd) == NULL,
	       "Can only have one main thread per CPU.");
	PCPU_SET(maintd, td);

	/*
	 * Make sure we are only ever on this CPU.
	 */
	scheduler_cpu_pin(td);
	SCHEDULER_UNLOCK();
}

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

	scheduler_queue(NULL, se);
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
	struct scheduler_queue *sq = PCPU_GET(scheduler);

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
scheduler_schedule(void)
{
	struct thread *td;

	SCHEDULER_LOCK();
	/*
	 * Find a thread to run.  If there's nothing in the runqueue, check to
	 * see if we're the main thread.  If we are the main thread, yield the
	 * CPU until we get an interrupt.  Otherwise, without a thread to run,
	 * switch to the main thread.
	 */
	td = scheduler_pick_thread();
	if (td == NULL) {
		if (PCPU_GET(maintd) == current_thread()) {
			SCHEDULER_UNLOCK();
			scheduler_yield();
			return;
		}
		td = PCPU_GET(maintd);
	}
	scheduler_switch(td);
}

void
scheduler_thread_runnable(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	scheduler_queue(NULL, se);
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
	SQ_LOCK(&scheduler_sleep_queue);
	scheduler_queue(&scheduler_sleep_queue, se);
	SQ_UNLOCK(&scheduler_sleep_queue);
	SCHEDULER_UNLOCK();
}

static struct scheduler_entry *
scheduler_pick_entry(struct scheduler_queue *sq)
{
	struct scheduler_entry *se;

	SQ_LOCK(sq);

	/*
	 * If this queue can not yet be used to switch threads, return the
	 * main thread.
	 */
	if (!sq->sq_switchable) {
		ASSERT(PCPU_GET(maintd) != NULL, "Need a main thread.");
		SQ_UNLOCK(sq);
		return (&PCPU_GET(maintd)->td_sched);
	}

	TAILQ_FOREACH(se, &sq->sq_queue, se_link) {
		if (se->se_oncpu == CPU_ID_INVALID ||
		    se->se_oncpu == mp_whoami())
			break;
		
	}
	if (se == NULL) {
		SQ_UNLOCK(sq);
		return (NULL);
	}

	/*
	 * Push to tail of list.
	 */
	scheduler_queue(sq, se);

	SQ_UNLOCK(sq);

	return (se);
}

static struct thread *
scheduler_pick_thread(void)
{
	struct scheduler_entry *se;

	se = scheduler_pick_entry(PCPU_GET(scheduler));
	if (se == NULL)
		return (NULL);
	return (se->se_thread);
}

static struct scheduler_queue *
scheduler_pick_queue(struct scheduler_entry *se)
{
	struct scheduler_queue *sq, *winner;

	SCHEDULER_LOCK();
	if ((se->se_flags & SCHEDULER_PINNED) != 0) {
		sq = se->se_queue;
		ASSERT(sq != NULL, "Pinned thread must be on a queue.");
		SCHEDULER_UNLOCK();
		return (sq);
	}
	winner = NULL;
	TAILQ_FOREACH(sq, &scheduler_queue_list, sq_link) {
		SQ_LOCK(sq);
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
	SCHEDULER_UNLOCK();
	return (winner);
}

static void
scheduler_queue(struct scheduler_queue *sq, struct scheduler_entry *se)
{
	struct scheduler_queue *osq;

	/*
	 * The main thread cannot go on a queue.
	 */
	if ((se->se_flags & SCHEDULER_MAIN) != 0)
		return;

	if (sq == NULL)
		sq = scheduler_pick_queue(se);

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
	SQ_LOCK(sq);
	sq->sq_cpu = cpu;
	TAILQ_INIT(&sq->sq_queue);
	sq->sq_length = 0;
	sq->sq_switchable = false;
	if (sq != &scheduler_sleep_queue) {
		SCHEDULER_LOCK();
		TAILQ_INSERT_HEAD(&scheduler_queue_list, sq, sq_link);
		SCHEDULER_UNLOCK();
	}
	SQ_UNLOCK(sq);
}

static void
scheduler_switch(struct thread *td)
{
	struct scheduler_entry *se;
	struct thread *otd;

	SPINLOCK_ASSERT_HELD(&scheduler_lock);

	otd = current_thread();
	se = &td->td_sched;

	if (otd != NULL) {
		struct scheduler_entry *ose;

		ose = &otd->td_sched;

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
	se->se_oncpu = mp_whoami();
	SCHEDULER_UNLOCK();
	if (otd != td)
		thread_switch(otd, td);
}

static void
scheduler_yield(void)
{
	//cpu_scheduler_yield();
}

static void
scheduler_db_dump_queue(struct scheduler_queue *sq)
{
	struct scheduler_entry *se;

	kcprintf("Q%p => cpu%u\n", sq, sq->sq_cpu);
	TAILQ_FOREACH(se, &sq->sq_queue, se_link) {
		struct thread *td;

		td = se->se_thread;

		kcprintf("E%p => td=%p, %s\n", se, td, td->td_name);
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
