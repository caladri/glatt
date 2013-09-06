#include <core/types.h>
#include <core/mutex.h>
#include <core/sleepq.h>
#include <core/thread.h>

#define	MTX_SPINLOCK(mtx)	spinlock_lock(&(mtx)->mtx_lock)
#define	MTX_SPINUNLOCK(mtx)	spinlock_unlock(&(mtx)->mtx_lock)

void
mutex_init(struct mutex *mtx, const char *name, unsigned flags)
{
	spinlock_init(&mtx->mtx_lock, name, SPINLOCK_FLAG_DEFAULT);
	sleepq_init(&mtx->mtx_sleepq, &mtx->mtx_lock);
	mtx->mtx_owner = NULL;
	mtx->mtx_nested = 0;
	mtx->mtx_waiters = 0;
	mtx->mtx_flags = flags;
}

void
mutex_lock(struct mutex *mtx)
{
	unsigned int tries;
	struct thread *td;

	td = current_thread();
	ASSERT(td != NULL, "Must have a thread.");
	ASSERT(mtx != NULL, "Cannot lock NULL mutex.");
	/*
	 * Don't check this assertion for now, since there's nothing wrong
	 * with acquiring a mutex in a critical section if you're certain that
	 * you will not block.  The sleepq code ought to be able to check this
	 * more accurately?
	 */
#if 0
	ASSERT(!critical_section(),
	       "Cannot lock a mutex from within a critical section.");
#endif

	tries = 0;

	for (;;) {
		MTX_SPINLOCK(mtx);
		if (mtx->mtx_owner == td) {
			if ((mtx->mtx_flags & MUTEX_FLAG_RECURSE) != 0) {
				mtx->mtx_nested++;
				MTX_SPINUNLOCK(mtx);
				return;
			}
			panic("%s: cannot recurse on mutex %s", __func__,
			      mtx->mtx_lock.s_name);
		}
		if (mtx->mtx_owner == NULL) {
			mtx->mtx_nested++;
			mtx->mtx_owner = td;
			MTX_SPINUNLOCK(mtx);
			return;
		}
		if (tries++ < 10) {
			MTX_SPINUNLOCK(mtx);
			/* Try spinning for a while.  */
			continue;
		}
		mtx->mtx_waiters++;
		sleepq_enter(&mtx->mtx_sleepq);
		mtx->mtx_waiters--;
	}
}

void
mutex_unlock(struct mutex *mtx)
{
	struct thread *td;

	td = current_thread();

	ASSERT(td != NULL, "Must have a thread.");

	MTX_SPINLOCK(mtx);
	ASSERT(mtx->mtx_owner == td, "Not my lock to unlock.");
	if (mtx->mtx_nested-- == 1) {
		mtx->mtx_owner = NULL;
		if (mtx->mtx_waiters != 0)
			sleepq_signal_one(&mtx->mtx_sleepq);
	}
	MTX_SPINUNLOCK(mtx);
}
