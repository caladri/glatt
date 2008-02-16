#ifndef	_CORE_MUTEX_H_
#define	_CORE_MUTEX_H_

#include <core/spinlock.h>

struct thread;

struct mutex {
	struct spinlock mtx_lock;
	struct thread *mtx_owner;
	unsigned mtx_nested;
};

struct mutex *mutex_allocate(const char *);
void mutex_init(struct mutex *, const char *);
void mutex_lock(struct mutex *);
bool mutex_try_lock(struct mutex *);
void mutex_unlock(struct mutex *);

#endif	/* !_CORE_MUTEX_H_ */
