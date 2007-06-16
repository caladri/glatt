#include <core/types.h>
#include <core/mutex.h>
#include <core/sleepq.h>
#include <core/thread.h>

#define	MTX_SPINLOCK(mtx)	spinlock_lock(&(mtx)->mtx_lock)
#define	MTX_SPINUNLOCK(mtx)	spinlock_unlock(&(mtx)->mtx_lock)

void
mutex_init(struct mutex *mtx, const char *name)
{
	spinlock_init(&mtx->mtx_lock, name);
	MTX_SPINLOCK(mtx);
	mtx->mtx_owner = NULL;
	mtx->mtx_nested = 0;
	MTX_SPINUNLOCK(mtx);
}

void
mutex_lock(struct mutex *mtx)
{
	ASSERT(current_thread() != NULL, "Must have a thread.");

	for (;;) {
		if (mutex_try_lock(mtx))
			return;
		sleepq_wait(mtx);
	}
}

bool
mutex_try_lock(struct mutex *mtx)
{
	struct thread *td;

	td = current_thread();

	ASSERT(td != NULL, "Must have a thread.");

	ASSERT(mtx != NULL, "Cannot lock NULL mutex.");

	MTX_SPINLOCK(mtx);
	if (mtx->mtx_owner == td) {
		mtx->mtx_nested++;
		MTX_SPINUNLOCK(mtx);
		return (true);
	}
	if (mtx->mtx_owner == NULL) {
		mtx->mtx_nested++;
		mtx->mtx_owner = td;
		MTX_SPINUNLOCK(mtx);
		return (true);
	}
	MTX_SPINUNLOCK(mtx);
	return (false);
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
		sleepq_signal_one(mtx);
	}
	MTX_SPINUNLOCK(mtx);
}
