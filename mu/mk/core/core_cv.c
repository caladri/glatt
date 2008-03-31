#include <core/types.h>
#include <core/cv.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/thread.h>

/*
 * XXX
 * Implement a more reliable condition variable protocol.
 */

struct cv {
	struct spinlock cv_spinlock;
	volatile bool cv_signalled;
};

static struct pool cv_pool;

#define	CV_LOCK(cv)	spinlock_lock(&(cv)->cv_spinlock)
#define	CV_UNLOCK(cv)	spinlock_unlock(&(cv)->cv_spinlock)

struct cv *
cv_create(void)
{
	struct cv *cv;

	cv = pool_allocate(&cv_pool);
	spinlock_init(&cv->cv_spinlock, "CV");
	cv->cv_signalled = false;
	/*TAILQ_INIT(&cv->cv_waiters);*/
	return (cv);
}

void
cv_signal(struct cv *cv)
{
	CV_LOCK(cv);
	if (cv->cv_signalled) {
		CV_UNLOCK(cv);
		return;
	}
	cv->cv_signalled = true;
	sleepq_signal_one(cv);
	/* XXX waiters */
	CV_UNLOCK(cv);
}

void
cv_signal_broadcast(struct cv *cv)
{
	CV_LOCK(cv);
	if (cv->cv_signalled) {
		CV_UNLOCK(cv);
		return;
	}
	cv->cv_signalled = true;
	sleepq_signal(cv);
	/* XXX waiters */
	CV_UNLOCK(cv);
}

void
cv_wait(struct cv *cv, struct mutex *mtx)
{
	for (;;) {
		CV_LOCK(cv);
		if (mtx != NULL) {
			mutex_unlock(mtx);
			mtx = NULL;
		}
		if (cv->cv_signalled) {
			cv->cv_signalled = false;
			CV_UNLOCK(cv);
			return;
		}
		sleepq_enter(cv, &cv->cv_spinlock);
	}
}

static void
cv_startup(void *arg)
{
	int error;

	error = pool_create(&cv_pool, "CV", sizeof (struct cv), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(cv, STARTUP_POOL, STARTUP_FIRST, cv_startup, NULL);
