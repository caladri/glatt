#ifndef	_CORE_MUTEX_H_
#define	_CORE_MUTEX_H_

#include <core/spinlock.h>

struct thread;

struct mutex {
	struct spinlock mtx_lock;
	struct thread *mtx_owner;
	unsigned mtx_nested;
};

#define	MTX_HELD(mtx)	((mtx)->mtx_owner != NULL && (mtx)->mtx_owner == current_thread())

void mutex_init(struct mutex *, const char *);
void mutex_lock(struct mutex *);
void mutex_unlock(struct mutex *);

#endif	/* !_CORE_MUTEX_H_ */
