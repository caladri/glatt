#include <core/types.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/spinlock.h>
#include <core/thread.h>
#ifdef DB
#include <db/db_command.h>
#include <core/console.h>
#endif

#ifdef DB
DB_COMMAND_TREE(scheduler, root, scheduler);
#endif

struct scheduler_queue {
	TAILQ_HEAD(, struct scheduler_entry) sq_queue;
	TAILQ_HEAD(, struct scheduler_entry) sq_exiting;
};

static struct spinlock scheduler_lock;
static struct scheduler_queue scheduler_queue;

#define	SCHEDULER_LOCK()	spinlock_lock(&scheduler_lock)
#define	SCHEDULER_UNLOCK()	spinlock_unlock(&scheduler_lock)
#define	SCHEDULER_ASSERT_LOCKED()					\
	SPINLOCK_ASSERT_HELD(&scheduler_lock)

static void scheduler_switch(struct scheduler_entry *, struct scheduler_entry *);

void
scheduler_init(void)
{
	spinlock_init(&scheduler_lock, "SCHEDULER", SPINLOCK_FLAG_DEFAULT);
	TAILQ_INIT(&scheduler_queue.sq_queue);
	TAILQ_INIT(&scheduler_queue.sq_exiting);
}

void
scheduler_activate(struct thread *td)
{
	/*
	 * First execution, we have to unlock the spinlock by hand.  In future
	 * calls, it will be handled by scheduler_switch().
	 */
	SCHEDULER_UNLOCK();
}

void
scheduler_cpu_pin(struct thread *td)
{
#ifndef UNIPROCESSOR
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_LOCK();
	if ((se->se_flags & (SCHEDULER_RUNNING | SCHEDULER_RUNNABLE)) != 0)
		panic("%s: called for running and/or runnable thread.", __func__);
	if ((se->se_flags & SCHEDULER_PINNED) != 0)
		panic("%s: thread already pinned.", __func__);
	se->se_flags |= SCHEDULER_PINNED;
	se->se_oncpu = mp_whoami();
	SCHEDULER_UNLOCK();
#endif
}

/*
 * Determine if any threads other than the current one are runnable
 * on this CPU.
 */
bool
scheduler_idle(void)
{
	struct scheduler_entry *ose, *se;

	SCHEDULER_LOCK();
	ose = &current_thread()->td_sched;
	TAILQ_FOREACH(se, &scheduler_queue.sq_queue, se_link) {
		if (se == ose)
			continue;
		if ((se->se_flags & SCHEDULER_RUNNABLE) == 0)
			continue;
		if ((se->se_flags & SCHEDULER_RUNNING) != 0)
			continue;
#ifndef UNIPROCESSOR
		if ((se->se_flags & SCHEDULER_PINNED) != 0) {
			if (se->se_oncpu != mp_whoami())
				continue;
		}
#endif
		SCHEDULER_UNLOCK();
		return (false);
	}
	SCHEDULER_UNLOCK();
	return (true);
}

void
scheduler_schedule(struct thread *td, struct spinlock *lock)
{
	struct scheduler_entry *ose, *se;
	bool canrun;

	SCHEDULER_LOCK();
	ose = current_thread() == NULL ? NULL : &current_thread()->td_sched;
	canrun = ose == NULL ? false :
		(ose->se_flags & SCHEDULER_RUNNABLE) != 0;
	if (lock != NULL)
		spinlock_unlock(lock);

	/*
	 * Free exiting threads which are no longer running.
	 */
restart:
	TAILQ_FOREACH(se, &scheduler_queue.sq_exiting, se_link) {
		ASSERT((se->se_flags & SCHEDULER_EXITING) != 0,
		       "exiting thread must not be in queue.");
		if ((se->se_flags & SCHEDULER_RUNNING) != 0)
			continue;
		thread_free(se->se_thread);
		goto restart;
	}

	TAILQ_FOREACH(se, &scheduler_queue.sq_queue, se_link) {
		ASSERT((se->se_flags & SCHEDULER_EXITING) == 0,
		       "exiting thread must not be in queue.");
		if ((se->se_flags & SCHEDULER_RUNNABLE) == 0)
			continue;
		if ((se->se_flags & SCHEDULER_RUNNING) != 0)
			continue;
#ifndef UNIPROCESSOR
		if ((se->se_flags & SCHEDULER_PINNED) != 0) {
			if (se->se_oncpu != mp_whoami())
				continue;
		}
#endif
		if (td != NULL && se->se_thread != td)
			continue;
		scheduler_switch(ose, se);
		return;
	}
	if (canrun) {
		scheduler_switch(ose, ose);
		return;
	}
	SCHEDULER_UNLOCK();
	panic("%s: no threads are runnable.", __func__);
}

void
scheduler_thread_exiting(void)
{
	struct thread *td = current_thread();
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_LOCK();
	if ((se->se_flags & SCHEDULER_EXITING) != 0)
		panic("%s: thread already exiting.", __func__);
	se->se_flags &= ~SCHEDULER_RUNNABLE;
	se->se_flags |= SCHEDULER_EXITING;

	TAILQ_REMOVE(&scheduler_queue.sq_queue, se, se_link);
	TAILQ_INSERT_TAIL(&scheduler_queue.sq_exiting, se, se_link);

	SCHEDULER_UNLOCK();
}

void
scheduler_thread_free(struct thread *td)
{
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_ASSERT_LOCKED();

	if ((se->se_flags & SCHEDULER_EXITING) == 0)
		panic("%s: thread not exiting.", __func__);
	TAILQ_REMOVE(&scheduler_queue.sq_exiting, se, se_link);
}

void
scheduler_thread_runnable(struct thread *td)
{
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_LOCK();
#if 0
	if ((se->se_flags & SCHEDULER_RUNNABLE) != 0)
		panic("%s: thread already runnable.", __func__);
#endif
	se->se_flags &= ~SCHEDULER_SLEEPING;
	se->se_flags |= SCHEDULER_RUNNABLE;
	SCHEDULER_UNLOCK();
}

void
scheduler_thread_setup(struct thread *td)
{
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_LOCK();
	se->se_thread = td;
	se->se_flags = SCHEDULER_DEFAULT;
#ifndef UNIPROCESSOR
	se->se_oncpu = CPU_ID_INVALID;
#endif
	TAILQ_INSERT_TAIL(&scheduler_queue.sq_queue, se, se_link);
	SCHEDULER_UNLOCK();
}

void
scheduler_thread_sleeping(struct thread *td)
{
	struct scheduler_entry *se = &td->td_sched;

	SCHEDULER_LOCK();
	if ((se->se_flags & SCHEDULER_SLEEPING) != 0)
		panic("%s: thread already sleeping.", __func__);
	se->se_flags &= ~SCHEDULER_RUNNABLE;
	se->se_flags |= SCHEDULER_SLEEPING;
	SCHEDULER_UNLOCK();
}

static void
scheduler_switch(struct scheduler_entry *ose, struct scheduler_entry *se)
{
	struct thread *otd, *td;

	SCHEDULER_ASSERT_LOCKED();

	otd = ose == NULL ? NULL : ose->se_thread;
	td = se->se_thread;

	TAILQ_REMOVE(&scheduler_queue.sq_queue, se, se_link);
	TAILQ_INSERT_TAIL(&scheduler_queue.sq_queue, se, se_link);
	if (ose != NULL) {
		ose->se_flags &= ~SCHEDULER_RUNNING;
#ifndef UNIPROCESSOR
		if ((ose->se_flags & SCHEDULER_PINNED) == 0)
			se->se_oncpu = CPU_ID_INVALID;
#endif
	}
	se->se_flags |= SCHEDULER_RUNNING;
#ifndef UNIPROCESSOR
	se->se_oncpu = mp_whoami();
#endif
	if (otd != td)
		thread_switch(otd, td);
	SCHEDULER_UNLOCK();
}

#ifdef DB
static void
scheduler_db_dump_queue(struct scheduler_queue *sq)
{
	struct scheduler_entry *se;

	TAILQ_FOREACH(se, &sq->sq_queue, se_link) {
		struct thread *td;

		td = se->se_thread;

		kcprintf("%p (thread %p, \"%s\")%s%s%s%s",
			 se, td, td->td_name,
			 ((se->se_flags & SCHEDULER_RUNNING) ?
			  " running" : ""),
#ifndef UNIPROCESSOR
			 ((se->se_flags & SCHEDULER_PINNED) ?
			  " pinned" : ""),
#else
			 "",
#endif
			 ((se->se_flags & SCHEDULER_SLEEPING) ?
			  " sleeping" : ""),
			 ((se->se_flags & SCHEDULER_RUNNABLE) ?
			  " runnable" : ""));
#ifndef UNIPROCESSOR
		kcprintf(" cpu%u", se->se_oncpu);
#endif
		kcprintf("\n");
	}
}

static void
scheduler_db_dump(void)
{
	kcprintf("Dumping scheduler queue...\n");
	scheduler_db_dump_queue(&scheduler_queue);
}
DB_COMMAND(queues, scheduler, scheduler_db_dump);
#endif
