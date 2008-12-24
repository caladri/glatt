#ifndef	_CORE_MUTEX_H_
#define	_CORE_MUTEX_H_

#include <core/spinlock.h>

struct thread;

struct mutex {
	struct spinlock mtx_lock;
	struct thread *mtx_owner;
	unsigned mtx_nested;
	unsigned mtx_flags;
};

#define	MUTEX_FLAG_DEFAULT	(0x00000000)
#define	MUTEX_FLAG_RECURSE	(0x00000001)

#define	MUTEX_HELD(mtx)							\
	((mtx)->mtx_owner != NULL && (mtx)->mtx_owner == current_thread())

void mutex_init(struct mutex *, const char *, unsigned);
void mutex_lock(struct mutex *);
void mutex_unlock(struct mutex *);

#endif	/* !_CORE_MUTEX_H_ */
