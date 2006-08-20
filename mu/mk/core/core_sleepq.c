#include <core/types.h>
#include <core/alloc.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/thread.h>

static struct spinlock sleepq_lock = SPINLOCK_INIT("SLEEPQ");

#define	SLEEPQ_LOCK()	spinlock_lock(&sleepq_lock)
#define	SLEEPQ_UNLOCK()	spinlock_unlock(&sleepq_lock)

struct sleepq_entry {
	struct thread *se_thread;
	STAILQ_ENTRY(struct sleepq_entry) se_link;
};

static struct pool sleepq_entry_pool = POOL_INIT("SLEEPQ ENTRY", struct sleepq_entry, POOL_VIRTUAL);

struct sleepq {
	struct spinlock sq_lock;
	const void *sq_cookie;
	STAILQ_HEAD(, struct sleepq_entry) sq_entries;
	STAILQ_ENTRY(struct sleepq) sq_link;
};

static struct pool sleepq_pool = POOL_INIT("SLEEPQ", struct sleepq, POOL_VIRTUAL);

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

static STAILQ_HEAD(, struct sleepq) sleep_queue_list = STAILQ_HEAD_INITIALIZER(sleep_queue_list);

static struct sleepq *sleepq_lookup(const void *, bool);
static void sleepq_signal_first(struct sleepq *);

void
sleepq_signal(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	while (!STAILQ_EMPTY(&sq->sq_entries))
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
	STAILQ_INSERT_TAIL(&sq->sq_entries, se, se_link);
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
	STAILQ_FOREACH(sq, &sleep_queue_list, sq_link) {
		if (sq->sq_cookie == cookie) {
			SQ_LOCK(sq);
			SLEEPQ_UNLOCK();
			return (sq);
		}
	}
	if (!create)
		return (NULL);
	sq = pool_allocate(&sleepq_pool);
	spinlock_init(&sq->sq_lock, "SLEEP QUEUE");
	SQ_LOCK(sq);
	sq->sq_cookie = cookie;
	STAILQ_INIT(&sq->sq_entries);
	STAILQ_INSERT_TAIL(&sleep_queue_list, sq, sq_link);
	SLEEPQ_UNLOCK();
	return (sq);
}

static void
sleepq_signal_first(struct sleepq *sq)
{
	struct sleepq_entry *se;
	struct thread *td;

	/* XXX assert lock is held.  */
	se = STAILQ_FIRST(&sq->sq_entries);
	STAILQ_REMOVE(&sq->sq_entries, se, struct sleepq_entry, se_link);
	td = se->se_thread;
	pool_free(&sleepq_entry_pool, se);
	scheduler_thread_runnable(td);
}
