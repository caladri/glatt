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
	bool se_needwakeup;
	bool se_sleeping;
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
static void sleepq_signal_entry(struct sleepq *, struct sleepq_entry *);

static struct sleepq_entry *sleepq_entry_lookup(struct sleepq *, struct thread *);

void
sleepq_enter(const void *cookie)
{
	struct sleepq *sq;
	struct sleepq_entry *se;
	struct thread *td;

	td = current_thread();

	sq = sleepq_lookup(cookie, true);
	se = pool_allocate(&sleepq_entry_pool);
	se->se_thread = td;
	se->se_needwakeup = true;
	se->se_sleeping = false;
	TAILQ_INSERT_TAIL(&sq->sq_entries, se, se_link);
	SQ_UNLOCK(sq);
}

void
sleepq_signal(const void *cookie)
{
	struct sleepq_entry *se;
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	TAILQ_FOREACH(se, &sq->sq_entries, se_link) {
		sleepq_signal_entry(sq, se);
	}
	SQ_UNLOCK(sq);
}

void
sleepq_signal_one(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	if (!TAILQ_EMPTY(&sq->sq_entries)) {
		sleepq_signal_entry(sq, TAILQ_FIRST(&sq->sq_entries));
	}
	SQ_UNLOCK(sq);
}

void
sleepq_wait(const void *cookie)
{
	struct sleepq *sq;
	struct sleepq_entry *se;
	struct thread *td;

	td = current_thread();

	if (critical_section()) {
		panic("%s: thread (%s) tried to wait on a sleepq while in a critical section.", __func__, td->td_name);
	}

	sq = sleepq_lookup(cookie, false);

	ASSERT(sq != NULL, "Can't find an entry for a non-existant queue.");

	se = sleepq_entry_lookup(sq, td);
	if (se->se_needwakeup) {
		se->se_sleeping = true;
		scheduler_thread_sleeping(td);
		SQ_UNLOCK(sq);
		scheduler_schedule();
		SQ_LOCK(sq);
		se->se_sleeping = false;
	}
	TAILQ_REMOVE(&sq->sq_entries, se, se_link);
	SQ_UNLOCK(sq);
	ASSERT(!se->se_sleeping, "Thread woken up must be awake.");
	ASSERT(!se->se_needwakeup, "Thread woken up must not need it again.");
	pool_free(se);
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

#include<io/device/console/console.h>
static void
sleepq_signal_entry(struct sleepq *sq, struct sleepq_entry *se)
{
	struct thread *td;

	SPINLOCK_ASSERT_HELD(&sq->sq_lock);

	td = se->se_thread;
	if (se->se_needwakeup) {
		se->se_needwakeup = false;
		if (se->se_sleeping)
			scheduler_thread_runnable(td);
	} else {
		if (se->se_sleeping)
			kcprintf("%s: thread woken up twice for %p\n", __func__,
				 sq->sq_cookie);
	}
}

static struct sleepq_entry *
sleepq_entry_lookup(struct sleepq *sq, struct thread *td)
{
	struct sleepq_entry *se;

	TAILQ_FOREACH(se, &sq->sq_entries, se_link) {
		if (se->se_thread != td)
			continue;
		return (se);
	}
	ASSERT(false, "Must have an entry.");
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
