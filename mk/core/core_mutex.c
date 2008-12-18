#include <core/types.h>
#include <core/mutex.h>
#include <core/sleepq.h>
#include <core/thread.h>

#define	MTX_SPINLOCK(mtx)	spinlock_lock(&(mtx)->mtx_lock)
#define	MTX_SPINUNLOCK(mtx)	spinlock_unlock(&(mtx)->mtx_lock)

/*
 * Set the odd bit of the mutex address when turning it into a sleepq address.
 * If the mutex is embedded at the start of a structure that is slept on, we
 * could make or generate spurious wakeups and waits.  Nothing should be
 * sleeping on something internal to the mutex (and misaligned.)
 */
#define	MTX_CHANNEL(mtx)	((const void *)((uintptr_t)(mtx) | 1))

void
mutex_init(struct mutex *mtx, const char *name, unsigned flags)
{
	spinlock_init(&mtx->mtx_lock, name, SPINLOCK_FLAG_DEFAULT);
	MTX_SPINLOCK(mtx);
	mtx->mtx_owner = NULL;
	mtx->mtx_nested = 0;
	mtx->mtx_flags = flags;
	MTX_SPINUNLOCK(mtx);
}

void
mutex_lock(struct mutex *mtx)
{
	unsigned int tries;
	struct thread *td;

	td = current_thread();
	ASSERT(td != NULL, "Must have a thread.");
	ASSERT(mtx != NULL, "Cannot lock NULL mutex.");

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
		sleepq_enter(MTX_CHANNEL(mtx), &mtx->mtx_lock);
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
		sleepq_signal_one(MTX_CHANNEL(mtx));
	}
	MTX_SPINUNLOCK(mtx);
}
