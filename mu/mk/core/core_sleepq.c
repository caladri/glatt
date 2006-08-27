#include <core/types.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/thread.h>

static struct spinlock sleepq_lock;

#define	SLEEPQ_LOCK()	spinlock_lock(&sleepq_lock)
#define	SLEEPQ_UNLOCK()	spinlock_unlock(&sleepq_lock)

struct sleepq_entry {
	struct thread *se_thread;
	TAILQ_ENTRY(struct sleepq_entry) se_link;
};

static struct pool sleepq_entry_pool;

struct sleepq {
	struct spinlock sq_lock;
	const void *sq_cookie;
	TAILQ_HEAD(, struct sleepq_entry) sq_entries;
	TAILQ_ENTRY(struct sleepq) sq_link;
};

static struct pool sleepq_pool;

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

static TAILQ_HEAD(, struct sleepq) sleepq_queue_list;

static struct sleepq *sleepq_lookup(const void *, bool);
static void sleepq_signal_first(struct sleepq *);

void
sleepq_signal(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	while (!TAILQ_EMPTY(&sq->sq_entries))
		sleepq_signal_first(sq);
	SQ_UNLOCK(sq);
}

void
sleepq_signal_one(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	if (!TAILQ_EMPTY(&sq->sq_entries))
		sleepq_signal_first(sq);
	SQ_UNLOCK(sq);
}

void
sleepq_wait(const void *cookie)
{
	struct sleepq *sq;
	struct sleepq_entry *se, *ep;
	struct thread *td;

	td = current_thread();

	sq = sleepq_lookup(cookie, true);
	se = pool_allocate(&sleepq_entry_pool);
	se->se_thread = td;
	TAILQ_INSERT_TAIL(&sq->sq_entries, se, se_link);
	SQ_UNLOCK(sq);
	scheduler_thread_sleeping(td);
	scheduler_schedule();
}

/* RETURNS SQ LOCKED.  */
static struct sleepq *
sleepq_lookup(const void *cookie, bool create)
{
	struct sleepq *sq;

	SLEEPQ_LOCK();
	TAILQ_FOREACH(sq, &sleepq_queue_list, sq_link) {
		if (sq->sq_cookie == cookie) {
			SQ_LOCK(sq);
			SLEEPQ_UNLOCK();
			return (sq);
		}
	}
	if (!create) {
		SLEEPQ_UNLOCK();
		return (NULL);
	}
	sq = pool_allocate(&sleepq_pool);
	spinlock_init(&sq->sq_lock, "SLEEP QUEUE");
	SQ_LOCK(sq);
	sq->sq_cookie = cookie;
	TAILQ_INIT(&sq->sq_entries);
	TAILQ_INSERT_TAIL(&sleepq_queue_list, sq, sq_link);
	SLEEPQ_UNLOCK();
	return (sq);
}

static void
sleepq_signal_first(struct sleepq *sq)
{
	struct sleepq_entry *se;
	struct thread *td;

	SPINLOCK_ASSERT_HELD(&sq->sq_lock);

	se = TAILQ_FIRST(&sq->sq_entries);
	TAILQ_REMOVE(&sq->sq_entries, se, se_link);
	td = se->se_thread;
	pool_free(se);
	scheduler_thread_runnable(td);
}

static void
sleepq_startup(void *arg)
{
	int error;

	spinlock_init(&sleepq_lock, "SLEEPQ");
	error = pool_create(&sleepq_entry_pool, "SLEEPQ ENTRY",
			    sizeof (struct sleepq_entry), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	error = pool_create(&sleepq_pool, "SLEEPQ", sizeof (struct sleepq),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	TAILQ_INIT(&sleepq_queue_list);
}
STARTUP_ITEM(sleepq, STARTUP_POOL, STARTUP_FIRST, sleepq_startup, NULL);
