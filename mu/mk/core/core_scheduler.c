#include <core/types.h>
#include <core/scheduler.h>
#include <core/thread.h>
#include <db/db.h>
#include <db/db_command.h>
#include <io/device/console/console.h>

static struct scheduler_queue *scheduler_queue_head;
static struct spinlock scheduler_lock = SPINLOCK_INIT("SCHEDULER");

#define	SCHEDULER_LOCK()	spinlock_lock(&scheduler_lock)
#define	SCHEDULER_UNLOCK()	spinlock_unlock(&scheduler_lock);

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

static struct thread *scheduler_pick(void);
static struct scheduler_queue *scheduler_pick_queue(struct scheduler_entry *);
static void scheduler_queue(struct scheduler_queue *, struct scheduler_entry *);
static void scheduler_switch(struct thread *);
static void scheduler_yield(void);

void
scheduler_cpu_idle(struct thread *td)
{
	struct scheduler_entry *se;

	SCHEDULER_LOCK();
	se = &td->td_sched;
	se->se_flags |= SCHEDULER_IDLE;
	ASSERT(se->se_queue == NULL, "Idle thread cannot be on a queue.");
	ASSERT(PCPU_GET(idletd) == NULL,
	       "Can only have one idle thread per CPU.");
	PCPU_SET(idletd, td);

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

void
scheduler_cpu_setup(struct scheduler_queue *sq)
{
	spinlock_init(&sq->sq_lock, "SCHEDULER QUEUE");
	SQ_LOCK(sq);
	sq->sq_cpu = mp_whoami();
	sq->sq_first = NULL;
	sq->sq_last = NULL;
	sq->sq_length = 0;
	SCHEDULER_LOCK();
	sq->sq_link = scheduler_queue_head;
	scheduler_queue_head = sq;
	SCHEDULER_UNLOCK();
	SQ_UNLOCK(sq);
}

void
scheduler_schedule(void)
{
	struct thread *td;

	SCHEDULER_LOCK();
	/*
	 * Find a thread to run.  If there's nothing in the runqueue, check to
	 * see if we're the idle thread.  If we are the idle thread, yield the
	 * CPU until we get an interrupt.  Otherwise, without a thread to run,
	 * switch to the idle thread.
	 */
	td = scheduler_pick();
	if (td == NULL) {
		if (PCPU_GET(idletd) == current_thread()) {
			SCHEDULER_UNLOCK();
			scheduler_yield();
			return;
		}
		td = PCPU_GET(idletd);
		ASSERT(td != current_thread(), "Duh?");
	}
	scheduler_switch(td);
}

void
scheduler_thread_runnable(struct thread *td)
{
	struct scheduler_entry *se;

	se = &td->td_sched;
	scheduler_queue(NULL, se);
}

void
scheduler_thread_setup(struct thread *td)
{
	struct scheduler_entry *se;

	se = &td->td_sched;

	se->se_next = NULL;
	se->se_thread = td;
	se->se_queue = NULL;
	se->se_prev = NULL;
	se->se_flags = SCHEDULER_DEFAULT;
	se->se_oncpu = CPU_ID_INVALID;

	/*
	 * XXX
	 * Caller must eventually call scheduler_thread_runnable(td).
	 */
}

static struct thread *
scheduler_pick(void)
{
	struct scheduler_entry *se;
	struct scheduler_queue *sq;

	sq = &PCPU_GET(scheduler);
	SQ_LOCK(sq);
	se = sq->sq_first;
	if (se == NULL) {
		SQ_UNLOCK(sq);
		return (NULL);
	}

	/*
	 * Push to tail of list.
	 */
	scheduler_queue(sq, se);

	/*
	 * Let the idle thread run.
	 */
	if (se->se_thread == current_thread())
		return (NULL);

	return (se->se_thread);
}

/* XXX RETURNS SQ LOCKED */
static struct scheduler_queue *
scheduler_pick_queue(struct scheduler_entry *se)
{
	struct scheduler_queue *sq, *winner;

	SCHEDULER_LOCK();
	if ((se->se_flags & SCHEDULER_PINNED) != 0) {
		sq = se->se_queue;
		ASSERT(sq != NULL, "Pinned thread must be on a queue.");
		SQ_LOCK(sq);
		return (sq);
	}
	winner = NULL;
	for (sq = scheduler_queue_head; sq != NULL; sq = sq->sq_link) {
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
	return (winner);
}

/*
 * XXX WILL UNLOCK sq.
 */
static void
scheduler_queue(struct scheduler_queue *sq, struct scheduler_entry *se)
{
	struct scheduler_queue *osq;

	/*
	 * sq MUST BE LOCKED. XXX
	 */

	/*
	 * The idle thread cannot go on a queue.
	 */
	if ((se->se_flags & SCHEDULER_IDLE) != 0) {
		if (sq != NULL)
			SQ_UNLOCK(sq);
		return;
	}

	if (sq == NULL)
		sq = scheduler_pick_queue(se);
	/*
	 * XXX
	 * We allow for se->se_queue == sq.  In that case, the se will be
	 * pushed to the end of sq.
	 */
	if (se->se_queue != NULL) {
		osq = se->se_queue;
		SQ_LOCK(osq);
		if (se->se_prev != NULL) {
			se->se_prev->se_next = se->se_next;
		} else {
			ASSERT(osq->sq_first == se,
			       "Entry with no previous item must be first.");
			osq->sq_first = se->se_next;;
		}
		if (se->se_next != NULL) {
			se->se_next->se_prev = se->se_prev;
		} else {
			ASSERT(osq->sq_last == se,
			       "Entry with no next item must be last.");
			osq->sq_last = se->se_prev;
		}
		ASSERT(osq->sq_first != se &&
		       osq->sq_last != se,
		       "Entry cannot be first or last.");
		se->se_prev = NULL;
		se->se_next = NULL;
		se->se_queue = NULL;
		osq->sq_length--;
		SQ_UNLOCK(osq);
	}
	se->se_queue = sq;
	se->se_prev = sq->sq_last;
	sq->sq_last = se;
	se->se_prev->se_next = se;
	if (sq->sq_first == NULL)
		sq->sq_first = se;
	sq->sq_length++;
	SQ_UNLOCK(sq);
}

static void
scheduler_switch(struct thread *td)
{
	struct scheduler_entry *se;
	struct thread *otd;

	/* XXX SCHEDULER_LOCK MUST BE HELD */

	otd = current_thread();
	se = &td->td_sched;

	if (otd != NULL) {
		struct scheduler_entry *ose;

		ose = &otd->td_sched;

		ose->se_flags &= ~SCHEDULER_RUNNING;
		if ((ose->se_flags & SCHEDULER_PINNED) == 0) {
			scheduler_queue(NULL, ose);
		}
	}
	if ((se->se_flags & SCHEDULER_PINNED) != 0) {
		if (se->se_oncpu != mp_whoami())
			panic("%s: cannot migrate pinned thread.", __func__);
	}
	se->se_flags |= SCHEDULER_RUNNING;
	se->se_oncpu = mp_whoami();
	SCHEDULER_UNLOCK();
	thread_switch(otd, td);
}

static void
scheduler_yield(void)
{
	/* XXX cpu_yield(); ? */
}

static void
scheduler_db_dump_queue(struct scheduler_queue *sq)
{
	struct scheduler_entry *se;

	kcprintf("Q%p => cpu%u\n", sq, sq->sq_cpu);
	for (se = sq->sq_first; se != NULL; se = se->se_next) {
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
	for (sq = scheduler_queue_head; sq != NULL; sq = sq->sq_link)
		scheduler_db_dump_queue(sq);
	kcprintf("Done.\n");
}
DB_COMMAND(scheduler_dump, scheduler_db_dump, "Dump all scheduler queues.");
