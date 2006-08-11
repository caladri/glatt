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
	struct sleepq_entry *se_next;
};

static struct pool sleepq_entry_pool = POOL_INIT("SLEEPQ ENTRY", struct sleepq_entry, POOL_VIRTUAL);

struct sleepq {
	struct spinlock sq_lock;
	struct sleepq_entry *sq_entry;
	const void *sq_cookie;
	struct sleepq *sq_link;
};

static struct pool sleepq_pool = POOL_INIT("SLEEPQ", struct sleepq, POOL_VIRTUAL);

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

static struct sleepq *sleepq_queue_head;

static struct sleepq *sleepq_lookup(const void *, bool);
static void sleepq_signal_first(struct sleepq *);

void
sleepq_signal(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
	if (sq == NULL)
		return;
	while (sq->sq_entry != NULL)
		sleepq_signal_first(sq);
	SQ_UNLOCK(sq);
}

void
sleepq_signal_one(const void *cookie)
{
	struct sleepq *sq;

	sq = sleepq_lookup(cookie, false);
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
	se->se_next = NULL;
	if (sq->sq_entry == NULL) {
		sq->sq_entry = se;
	} else {
		for (ep = sq->sq_entry; ep->se_next != NULL; ep = ep->se_next)
			continue;
		ASSERT(ep != NULL && ep->se_next == NULL, "Need queue tail.");
		ep->se_next = se;
	}
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
	for (sq = sleepq_queue_head; sq != NULL; sq = sq->sq_link) {
		if (sq->sq_cookie == cookie) {
			SQ_LOCK(sq);
			SLEEPQ_UNLOCK();
			return (sq);
		}
	}
	sq = pool_allocate(&sleepq_pool);
	spinlock_init(&sq->sq_lock, "SLEEP QUEUE");
	SQ_LOCK(sq);
	sq->sq_entry = NULL;
	sq->sq_cookie = cookie;
	sq->sq_link = sleepq_queue_head;
	sleepq_queue_head = sq;
	SLEEPQ_UNLOCK();
	return (sq);
}

static void
sleepq_signal_first(struct sleepq *sq)
{
	struct sleepq_entry *se;
	struct thread *td;

	se = sq->sq_entry;
	sq->sq_entry = se->se_next;
	td = se->se_thread;
	pool_free(&sleepq_entry_pool, se);
	scheduler_thread_runnable(td);
}
