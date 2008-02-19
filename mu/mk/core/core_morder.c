#include <core/types.h>
#include <core/morder.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/spinlock.h>
#include <core/string.h>
#include <core/thread.h>

#include <io/device/console/console.h>

struct morder_entry {
	struct morder *m_order;
	const char *m_class;
	SLIST_ENTRY(struct morder_entry) m_link;
	SLIST_ENTRY(struct morder_entry) m_alllink;
};

struct morder {
	SLIST_HEAD(, struct morder_entry) m_list;
	struct morder *m_child;
	unsigned m_level;
};

struct morder_lock {
	struct morder_entry *ml_order;
	struct mutex *ml_mutex;
	unsigned ml_count;
	SLIST_ENTRY(struct morder_lock) ml_link;
};

static struct spinlock morder_spinlock = SPINLOCK_INIT("MORDER");
static struct pool morder_pool =
	POOL_INIT("MORDER", struct morder, POOL_DEFAULT);
static struct pool morder_entry_pool =
	POOL_INIT("MORDER ENTRY", struct morder_entry, POOL_DEFAULT);
static struct pool morder_lock_pool =
	POOL_INIT("MORDER LOCK", struct morder_lock, POOL_DEFAULT);

static struct morder morder_bottom = {
	.m_child = NULL,
	.m_level = 0,
};
static SLIST_HEAD(, struct morder_entry) morder_allentries;

static struct morder_lock *morder_create(struct morder_thread *, struct mutex *);
static void morder_dispose(struct morder_thread *, struct morder_lock *);
static void morder_enter(struct mutex *);
static struct morder_entry *morder_insert(struct morder *, const char *);
static struct morder_lock *morder_lookup(struct morder_thread *, struct mutex *);
static struct morder_entry *morder_order(struct morder_thread *, struct mutex *);
static void morder_remove(struct mutex *);

void
morder_thread_setup(struct thread *td)
{
	SLIST_INIT(&td->td_morder.mt_locks);
}

void
morder_lock(struct mutex *mtx)
{
	spinlock_lock(&morder_spinlock);
	morder_enter(mtx);
	spinlock_unlock(&morder_spinlock);
}

void
morder_unlock(struct mutex *mtx)
{
	spinlock_lock(&morder_spinlock);
	morder_remove(mtx);
	spinlock_unlock(&morder_spinlock);
}

static struct morder_lock *
morder_create(struct morder_thread *mt, struct mutex *mtx)
{
	struct morder_lock *ml;

	ml = pool_allocate(&morder_lock_pool);
	ml->ml_order = morder_order(mt, mtx);
	ml->ml_mutex = mtx;
	ml->ml_count = 0;
	return (ml);
}

static void
morder_dispose(struct morder_thread *mt, struct morder_lock *ml)
{
	SLIST_REMOVE(&mt->mt_locks, ml, struct morder_lock, ml_link);
	pool_free(ml);
}

static void
morder_enter(struct mutex *mtx)
{
	struct morder_lock *ml, *mm;
	struct thread *td;

	td = current_thread();
	ml = morder_lookup(&td->td_morder, mtx);
	if (ml != NULL) {
		ml->ml_count++;
		return;
	}
	ml = morder_create(&td->td_morder, mtx);
	SLIST_FOREACH(mm, &td->td_morder.mt_locks, ml_link) {
		if (ml->ml_order->m_order->m_level <
		    mm->ml_order->m_order->m_level) {
			panic("%s: lock order reversal\n\t1st: %s\n\t2nd: %s", __func__, mm->ml_order->m_class, ml->ml_order->m_class);
		}
	}
	ml->ml_count++;
	SLIST_INSERT_HEAD(&td->td_morder.mt_locks, ml, ml_link);
}

static struct morder_entry *
morder_insert(struct morder *mo, const char *class)
{
	struct morder_entry *me;

	me = pool_allocate(&morder_entry_pool);
	me->m_order = mo;
	me->m_class = class;

	SLIST_INSERT_HEAD(&mo->m_list, me, m_link);
	SLIST_INSERT_HEAD(&morder_allentries, me, m_alllink);

	return (me);
}

static struct morder_lock *
morder_lookup(struct morder_thread *mt, struct mutex *mtx)
{
	struct morder_lock *ml;

	SLIST_FOREACH(ml, &mt->mt_locks, ml_link) {
		if (ml->ml_mutex == mtx)
			return (ml);
	}
	return (NULL);
}

static struct morder_entry *
morder_order(struct morder_thread *mt, struct mutex *mtx)
{
	struct morder_entry *me;
	struct morder_lock *ml;
	struct morder *mo, *mp;
	const char *class;

	class = mtx->mtx_lock.s_name;
	SLIST_FOREACH(me, &morder_allentries, m_alllink) {
		if (strcmp(class, me->m_class) == 0) {
			return (me);
		}
	}

	if (SLIST_EMPTY(&mt->mt_locks)) {
		me = morder_insert(&morder_bottom, class);
		return (me);
	}

	mp = &morder_bottom;
	SLIST_FOREACH(ml, &mt->mt_locks, ml_link) {
		struct morder *mq;

		mq = ml->ml_order->m_order;
		if (mq->m_level > mp->m_level)
			mp = mq;
	}
	mo = mp->m_child;
	if (mo != NULL) {
		me = morder_insert(mo, class);
		return (me);
	}
	mo = pool_allocate(&morder_pool);
	SLIST_INIT(&mo->m_list);
	mo->m_child = NULL;
	mo->m_level = mp->m_level + 1;
	mp->m_child = mo;
	me = morder_insert(mo, class);
	return (me);
}

static void
morder_remove(struct mutex *mtx)
{
	struct morder_lock *ml;
	struct thread *td;

	td = current_thread();
	ml = morder_lookup(&td->td_morder, mtx);
	if (ml == NULL) {
		panic("%s: lock (%s) not held.", __func__,
		      mtx->mtx_lock.s_name);
	}
	ml->ml_count--;
	if (ml->ml_count == 0)
		morder_dispose(&td->td_morder, ml);
}
