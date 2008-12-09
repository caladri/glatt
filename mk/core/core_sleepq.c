#include <core/types.h>
#include <core/btree.h>
#include <core/pool.h>
#include <core/queue.h>
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
	BTREE_NODE(struct sleepq) sq_tree;
};

static struct pool sleepq_pool;

#define	SQ_LOCK(sq)	spinlock_lock(&(sq)->sq_lock)
#define	SQ_UNLOCK(sq)	spinlock_unlock(&(sq)->sq_lock)

static BTREE_ROOT(struct sleepq) sleepq_queue_tree;

static struct sleepq *sleepq_lookup(const void *, bool);
static void sleepq_signal_entry(struct sleepq *, struct sleepq_entry *);

void
sleepq_enter(const void *cookie, struct spinlock *lock)
{
	struct sleepq *sq;
	struct sleepq_entry *se;
	struct thread *td;

	td = current_thread();

	sq = sleepq_lookup(cookie, true);
	if (lock != NULL)
		spinlock_unlock(lock);
	se = pool_allocate(&sleepq_entry_pool);
	se->se_thread = td;
	TAILQ_INSERT_TAIL(&sq->sq_entries, se, se_link);

	scheduler_thread_sleeping(td);
	scheduler_schedule(NULL, &sq->sq_lock);
	SQ_LOCK(sq);
	TAILQ_REMOVE(&sq->sq_entries, se, se_link);
	SQ_UNLOCK(sq);
	pool_free(se);
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

/* RETURNS SQ LOCKED.  */
static struct sleepq *
sleepq_lookup(const void *cookie, bool create)
{
	struct sleepq *sq, *iter;

	SLEEPQ_LOCK();
	BTREE_FIND(&sq, iter, &sleepq_queue_tree, sq_tree,
		   (cookie < iter->sq_cookie), (cookie == iter->sq_cookie));
	if (sq != NULL) {
		SQ_LOCK(sq);
		SLEEPQ_UNLOCK();
		return (sq);
	}
	if (!create) {
		SLEEPQ_UNLOCK();
		return (NULL);
	}
	sq = pool_allocate(&sleepq_pool);
	spinlock_init(&sq->sq_lock, "SLEEP QUEUE", SPINLOCK_FLAG_DEFAULT);
	SQ_LOCK(sq);
	sq->sq_cookie = cookie;
	TAILQ_INIT(&sq->sq_entries);
	BTREE_INIT(&sq->sq_tree);
	BTREE_INSERT(sq, iter, &sleepq_queue_tree, sq_tree,
		     (sq->sq_cookie < iter->sq_cookie));
	SLEEPQ_UNLOCK();
	return (sq);
}

static void
sleepq_signal_entry(struct sleepq *sq, struct sleepq_entry *se)
{
	struct thread *td;

	SPINLOCK_ASSERT_HELD(&sq->sq_lock);

	td = se->se_thread;
	scheduler_thread_runnable(td);
}

static void
sleepq_startup(void *arg)
{
	int error;

	spinlock_init(&sleepq_lock, "SLEEPQ", SPINLOCK_FLAG_DEFAULT);
	error = pool_create(&sleepq_entry_pool, "SLEEPQ ENTRY",
			    sizeof (struct sleepq_entry), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	error = pool_create(&sleepq_pool, "SLEEPQ", sizeof (struct sleepq),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	BTREE_INIT_ROOT(&sleepq_queue_tree);
}
STARTUP_ITEM(sleepq, STARTUP_POOL, STARTUP_FIRST, sleepq_startup, NULL);
