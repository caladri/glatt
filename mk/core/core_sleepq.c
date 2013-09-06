#include <core/types.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/thread.h>

struct sleepq_entry {
	struct thread *se_thread;
	TAILQ_ENTRY(struct sleepq_entry) se_link;
};

static struct pool sleepq_entry_pool;

static void sleepq_signal_entry(struct sleepq *, struct sleepq_entry *);

void
sleepq_init(struct sleepq *sq, struct spinlock *lock)
{
	sq->sq_lock = lock;
	TAILQ_INIT(&sq->sq_entries);
}

void
sleepq_enter(struct sleepq *sq)
{
	struct sleepq_entry *se;
	struct thread *td;

	td = current_thread();

	SPINLOCK_ASSERT_HELD(sq->sq_lock);

	se = pool_allocate(&sleepq_entry_pool);
	se->se_thread = td;

	TAILQ_INSERT_TAIL(&sq->sq_entries, se, se_link);

	scheduler_thread_sleeping(td);
	scheduler_schedule(NULL, sq->sq_lock);
	spinlock_lock(sq->sq_lock);
	TAILQ_REMOVE(&sq->sq_entries, se, se_link);
	spinlock_unlock(sq->sq_lock);
	pool_free(se);
}

void
sleepq_signal(struct sleepq *sq)
{
	struct sleepq_entry *se;

	SPINLOCK_ASSERT_HELD(sq->sq_lock);
	TAILQ_FOREACH(se, &sq->sq_entries, se_link)
		sleepq_signal_entry(sq, se);
}

void
sleepq_signal_one(struct sleepq *sq)
{
	SPINLOCK_ASSERT_HELD(sq->sq_lock);
	if (!TAILQ_EMPTY(&sq->sq_entries))
		sleepq_signal_entry(sq, TAILQ_FIRST(&sq->sq_entries));
}

static void
sleepq_signal_entry(struct sleepq *sq, struct sleepq_entry *se)
{
	struct thread *td;

	td = se->se_thread;
	scheduler_thread_runnable(td);
}

static void
sleepq_startup(void *arg)
{
	int error;

	error = pool_create(&sleepq_entry_pool, "SLEEPQ ENTRY",
			    sizeof (struct sleepq_entry), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(sleepq, STARTUP_POOL, STARTUP_FIRST, sleepq_startup, NULL);
