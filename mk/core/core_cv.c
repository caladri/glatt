#include <core/types.h>
#include <core/cv.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/thread.h>

struct cv {
	struct spinlock cv_spinlock;
	struct mutex *cv_mutex;
	struct sleepq cv_sleepq;
};

static struct pool cv_pool;

#define	CV_LOCK(cv)	spinlock_lock(&(cv)->cv_spinlock)
#define	CV_UNLOCK(cv)	spinlock_unlock(&(cv)->cv_spinlock)

#define	CV_ASSERT_MUTEX_HELD(cv)					\
	ASSERT(MUTEX_HELD((cv)->cv_mutex),				\
	       "Mutex must protect condition variable operations.")

struct cv *
cv_create(struct mutex *mtx)
{
	struct cv *cv;

	ASSERT(mtx != NULL, "Condition variable must be protected by a mutex.");
	cv = pool_allocate(&cv_pool);
	spinlock_init(&cv->cv_spinlock, "CV", SPINLOCK_FLAG_DEFAULT);
	cv->cv_mutex = mtx;
	sleepq_init(&cv->cv_sleepq, &cv->cv_spinlock);
	return (cv);
}

void
cv_signal(struct cv *cv)
{
	CV_ASSERT_MUTEX_HELD(cv);

	CV_LOCK(cv);
	sleepq_signal_one(&cv->cv_sleepq);
	CV_UNLOCK(cv);
}

void
cv_signal_broadcast(struct cv *cv)
{
	CV_ASSERT_MUTEX_HELD(cv);

	CV_LOCK(cv);
	sleepq_signal(&cv->cv_sleepq);
	CV_UNLOCK(cv);
}

void
cv_wait(struct cv *cv)
{
	CV_ASSERT_MUTEX_HELD(cv);

	CV_LOCK(cv);
	mutex_unlock(cv->cv_mutex);
	sleepq_enter(&cv->cv_sleepq);
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
